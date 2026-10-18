#!/usr/bin/env python3
"""Check BootPI's static ELF layout and the raw image copied to the Pi."""

from pathlib import Path
import struct
import sys


def require(condition, message):
    if not condition:
        raise ValueError(message)


def check(elf_path, image_path, config_path):
    elf = Path(elf_path).read_bytes()
    image = Path(image_path).read_bytes()

    def unpack(fmt, offset):
        require(0 <= offset <= len(elf) - struct.calcsize(fmt), "truncated ELF table")
        return struct.unpack_from(fmt, elf, offset)

    def contents(offset, size):
        require(0 <= offset <= len(elf) and size <= len(elf) - offset,
                "ELF contents outside file")
        return elf[offset:offset + size]

    def name_at(strings, offset):
        require(0 <= offset < len(strings), "invalid ELF string offset")
        end = strings.find(b"\0", offset)
        require(end >= 0, "unterminated ELF string")
        return strings[offset:end].decode("ascii")

    header = unpack("<16sHHIQQQIHHHHHH", 0)
    ident, kind, machine, version, entry, phoff, shoff, flags, ehsize, phsize, phnum, shsize, shnum, shstr = header
    require(ident[:7] == b"\x7fELF\x02\x01\x01", "expected little-endian ELF64")
    require((kind, machine, version) == (2, 183, 1), "expected AArch64 executable")
    require(flags == 0 and ehsize == 64 and phsize == 56 and shsize == 64,
            "unexpected ELF header layout")
    require(shnum > 0 and shstr < shnum, "missing section table")
    rows = [unpack("<IIQQQQIIQQ", shoff + i * shsize) for i in range(shnum)]
    strings = contents(rows[shstr][4], rows[shstr][5])
    sections = []
    for row in rows:
        name, stype, sflags, address, offset, size, link, info, alignment, entsize = row
        sections.append(dict(name=name_at(strings, name), type=stype, flags=sflags,
                             address=address, offset=offset, size=size, link=link,
                             alignment=alignment, entsize=entsize))
    named = {s["name"]: s for s in sections}

    require(not any(s["type"] in (4, 6, 9, 11) for s in sections),
            "dynamic linking or relocations remain")
    require(".symtab" in named, "missing static symbol table")
    symbols = {}
    table = named[".symtab"]
    require(table["entsize"] == 24 and table["size"] % 24 == 0,
            "invalid symbol table")
    require(table["link"] < len(sections), "invalid symbol string table")
    string_section = sections[table["link"]]
    symbol_strings = contents(string_section["offset"], string_section["size"])
    for offset in range(table["offset"], table["offset"] + table["size"], 24):
        name, info, other, index, value, size = unpack("<IBBHQQ", offset)
        name = name_at(symbol_strings, name)
        require(index != 0 or not name, f"unresolved symbol: {name}")
        require(index != 0xfff2, f"unallocated COMMON symbol: {name}")
        if name:
            symbols[name] = value

    needed = ("_start", "kernel_main", "__image_start", "__image_end",
              "__data_start", "__data_end", "__bss_start", "__bss_end",
              "__stack_bottom", "__stack_top", "__memory_end")
    require(all(name in symbols for name in needed), "missing startup/linker symbols")
    base = symbols["__image_start"]
    require(base == entry == symbols["_start"] == 0x80000, "entry/load address mismatch")

    allocated = [s for s in sections if s["flags"] & 2]
    require(all(s["name"] in (".text", ".rodata", ".data", ".bss", ".stack")
                for s in allocated), "unexpected allocated section")
    for previous, current in zip(sorted(allocated, key=lambda s: s["address"]),
                                 sorted(allocated, key=lambda s: s["address"])[1:]):
        require(previous["address"] + previous["size"] <= current["address"],
                "allocated sections overlap")
    for name in (".text", ".data", ".bss", ".stack"):
        require(name in named, f"missing {name}")
    text, data, bss, stack = (named[name] for name in (".text", ".data", ".bss", ".stack"))
    require(text["type"] == 1 and text["flags"] == 6 and text["address"] == base,
            "boot text is not first executable section")
    require(text["address"] <= symbols["kernel_main"] < text["address"] + text["size"],
            "kernel_main is outside text")
    require(data["type"] == 1 and symbols["__data_start"] == data["address"]
            and symbols["__data_end"] == data["address"] + data["size"],
            "initialized data symbols disagree with section")
    require(data["flags"] in ((3,) if data["size"] else (2, 3)),
            "unexpected data permissions")
    if ".rodata" in named:
        require(named[".rodata"]["type"] == 1 and named[".rodata"]["flags"] == 2,
                "unexpected read-only data type/permissions")
    require(symbols["__image_end"] == symbols["__data_end"] <= bss["address"],
            "image end overlaps BSS")
    for section, start, end in ((bss, "__bss_start", "__bss_end"),
                                (stack, "__stack_bottom", "__stack_top")):
        require(section["type"] == 8 and section["flags"] == 3,
                f"{section['name']} must be writable NOBITS")
        require(symbols[start] == section["address"]
                and symbols[end] == section["address"] + section["size"],
                f"{section['name']} symbols disagree with section")
        require(symbols[start] % 16 == symbols[end] % 16 == 0,
                f"{section['name']} is not 16-byte aligned")
    require(symbols["__bss_end"] <= symbols["__stack_bottom"], "BSS overlaps stack")
    require(stack["size"] == 16 * 1024 and symbols["__memory_end"] == symbols["__stack_top"],
            "stack reservation or memory end mismatch")

    segments = []
    segment_flags = []
    for i in range(phnum):
        ptype, pflags, offset, virtual, physical, filesz, memsz, alignment = unpack(
            "<IIQQQQQQ", phoff + i * phsize)
        require(ptype == 1, "unexpected non-LOAD program header")
        require(virtual == physical and physical >= base, "virtual/physical load mismatch")
        require(filesz <= memsz and pflags in (5, 6), "invalid segment size/permissions")
        contents(offset, filesz)
        segments.append((physical, memsz, offset, filesz))
        segment_flags.append(pflags)
    require(len(segments) == 2, "expected separate code and RAM segments")
    require(set(segment_flags) == {5, 6}, "expected RX code and RW RAM segments")
    segments.sort()
    require(segments[0][0] + segments[0][1] <= segments[1][0], "LOAD segments overlap")
    for section in allocated:
        if not section["size"]:
            continue
        matches = [p for p in segments if p[0] <= section["address"]
                   and section["address"] + section["size"] <= p[0] + p[1]]
        require(len(matches) == 1, f"{section['name']} is outside LOAD memory")
        if section["type"] != 8:
            physical, memsz, offset, filesz = matches[0]
            require(section["address"] - physical == section["offset"] - offset
                    and section["offset"] + section["size"] <= offset + filesz,
                    f"{section['name']} load bytes disagree with address")

    initialized = [s for s in allocated if s["type"] != 8 and s["size"]]
    end = max(s["address"] + s["size"] for s in initialized)
    require(end <= symbols["__image_end"], "initialized bytes extend beyond image end")
    require(len(image) == end - base, "raw image length includes missing or extra bytes")
    expected = bytearray(len(image))
    for section in initialized:
        start = section["address"] - base
        expected[start:start + section["size"]] = contents(section["offset"], section["size"])
    require(image == expected, "raw image differs from ELF contents/zero-filled gaps")

    config = {}
    active = True
    for line in Path(config_path).read_text().splitlines():
        line = line.split("#", 1)[0].strip()
        if line.startswith("["):
            active = line in ("[pi4]", "[all]")
        elif active and "=" in line:
            key, value = line.split("=", 1)
            config[key.strip()] = value.strip()
    require(config.get("arm_64bit") == "1" and config.get("kernel") == "kernel8.img",
            "firmware must select the 64-bit kernel image")
    require(int(config.get("kernel_address", "-1"), 0) == base,
            "firmware load address disagrees with linker")
    require(config.get("arm_peri_high") == "0" and config.get("kernel_old", "0") == "0"
            and "armstub" not in config, "unsupported peripheral map or ARM stub")
    print(f"ELF/image checks passed: AArch64, entry {entry:#x}, {len(image)} image bytes")


if __name__ == "__main__":
    if len(sys.argv) != 4:
        sys.exit("usage: check_elf.py ELF IMAGE CONFIG")
    try:
        check(*sys.argv[1:])
    except (OSError, ValueError, struct.error) as error:
        sys.exit(f"ELF/image check failed: {error}")
