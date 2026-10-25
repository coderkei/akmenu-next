"""Run the built ARM9 donor crypto under Unicorn against independent references.

Requires Python packages unicorn and cryptography. Unicorn may also be installed
in arm9/build/donor-check-deps. Run after make; no console keys or NAND dump are
used. The ELF's actual optimized functions are executed without NDS startup.
"""

import argparse
import hashlib
from pathlib import Path
import random
import struct
import sys

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "arm9/build/donor-check-deps"))

from cryptography.hazmat.primitives.ciphers import Cipher, algorithms, modes
from unicorn import Uc, UC_ARCH_ARM, UC_HOOK_CODE, UC_MODE_ARM
from unicorn.arm_const import (
    UC_ARM_REG_CPSR, UC_ARM_REG_LR, UC_ARM_REG_PC, UC_ARM_REG_SP,
    UC_ARM_REG_R0, UC_ARM_REG_R1, UC_ARM_REG_R2, UC_ARM_REG_R3,
)

MASK = (1 << 128) - 1
MAGIC = 0xFFFEFB4E295902582A680F5F1A4F3E79
KEY_Y = 0xE1A00005202DDD1DBD4DC4D30AB9DC76
ARGS = 0x02E00000
RETURN = 0x02F10000


def rotate_left(value, bits):
    return ((value << bits) | (value >> (128 - bits))) & MASK


class Arm9:
    def __init__(self, path):
        data = path.read_bytes()
        if data[:6] != b"\x7fELF\x01\x01":
            raise ValueError("Expected a little endian ELF32 ARM9 executable")
        self.cpu = Uc(UC_ARCH_ARM, UC_MODE_ARM)
        self.cpu.mem_map(0x01000000, 0x10000)  # ITCM
        self.cpu.mem_map(0x02000000, 0x1000000)  # main RAM and DTCM addresses
        phoff = struct.unpack_from("<I", data, 28)[0]
        phsize, phcount = struct.unpack_from("<HH", data, 42)
        for index in range(phcount):
            kind, offset, address, _, size, _, _, _ = struct.unpack_from(
                "<8I", data, phoff + index * phsize
            )
            if kind == 1 and size:
                self.cpu.mem_write(address, data[offset:offset + size])

        shoff = struct.unpack_from("<I", data, 32)[0]
        shsize, shcount = struct.unpack_from("<HH", data, 46)
        sections = [struct.unpack_from("<10I", data, shoff + i * shsize)
                    for i in range(shcount)]
        self.symbols = {}
        for section in sections:
            if section[1] != 2:  # SHT_SYMTAB
                continue
            strings = sections[section[6]]
            names = data[strings[4]:strings[4] + strings[5]]
            for offset in range(section[4], section[4] + section[5], section[9]):
                name, value = struct.unpack_from("<II", data, offset)
                text = names[name:names.find(b"\0", name)].decode()
                self.symbols[text] = value

        self.heap = 0x02D00000
        for name in ("memalign", "free", "swiSHA1Calc"):
            address = self.symbols[name] & ~1
            self.cpu.hook_add(UC_HOOK_CODE, self.hook, name,
                              begin=address, end=address)

    def hook(self, cpu, address, size, name):
        if name == "memalign":
            alignment = cpu.reg_read(UC_ARM_REG_R0)
            count = cpu.reg_read(UC_ARM_REG_R1)
            self.heap = (self.heap + alignment - 1) & ~(alignment - 1)
            cpu.reg_write(UC_ARM_REG_R0, self.heap)
            self.heap += count
        elif name == "swiSHA1Calc":
            output = cpu.reg_read(UC_ARM_REG_R0)
            source = cpu.reg_read(UC_ARM_REG_R1)
            count = cpu.reg_read(UC_ARM_REG_R2)
            cpu.mem_write(output, hashlib.sha1(cpu.mem_read(source, count)).digest())
        # Replace only allocation and BIOS SHA1; key scrambling, software AES,
        # and NAND counter arithmetic all execute from the compiled ELF.
        cpu.reg_write(UC_ARM_REG_PC, cpu.reg_read(UC_ARM_REG_LR))

    def call(self, name, *args):
        self.cpu.reg_write(UC_ARM_REG_CPSR, 0x13)
        self.cpu.reg_write(UC_ARM_REG_SP, 0x02FE0000)
        self.cpu.reg_write(UC_ARM_REG_LR, RETURN)
        for register, value in zip((UC_ARM_REG_R0, UC_ARM_REG_R1,
                                    UC_ARM_REG_R2, UC_ARM_REG_R3), args):
            self.cpu.reg_write(register, value)
        self.cpu.emu_start(self.symbols[name], RETURN, count=2000000)
        if self.cpu.reg_read(UC_ARM_REG_PC) != RETURN:
            raise RuntimeError(f"{name} did not return within the instruction limit")

    def put_int(self, address, value):
        self.cpu.mem_write(address, value.to_bytes(16, "little"))

    def get_int(self, address):
        return int.from_bytes(self.cpu.mem_read(address, 16), "little")


def check_keys(arm):
    rng = random.Random(63)
    values = [0, 1, (1 << 64) - 1, 1 << 64, MASK, MAGIC]
    values += [rotate_left(MAGIC + delta, 42) for delta in (-1, 0, 1)]
    values += [rng.getrandbits(128) for _ in range(512)]
    for value in values:
        arm.put_int(ARGS, value)
        arm.call("F_XY_reverse", ARGS, ARGS + 32)
        expected = (rotate_left(value, 128 - 42) - MAGIC) & MASK
        actual = arm.get_int(ARGS + 32)
        assert actual == expected, (
            f"F_XY_reverse({value:032x}): got {actual:032x}, expected {expected:032x}"
        )
        other = rng.getrandbits(128)
        arm.put_int(ARGS, value)
        arm.put_int(ARGS + 32, other)
        arm.call("F_XY", ARGS + 64, ARGS, ARGS + 32)
        expected = rotate_left(((value ^ other) + MAGIC) & MASK, 42)
        assert arm.get_int(ARGS + 64) == expected, "F_XY differs from 128-bit reference"
    print(f"PASS: {len(values)} reverse and {len(values)} forward key vectors")


def check_nand(arm):
    rng = random.Random(0x4E414E44)
    for case in range(16):
        console_id = rng.getrandbits(64)
        low, high = console_id & 0xFFFFFFFF, console_id >> 32
        key_x = int.from_bytes(struct.pack(
            "<4I", low, low ^ 0x24EE6906, high ^ 0xE65B601D, high
        ), "little")
        normal_key = rotate_left(((key_x ^ KEY_Y) + MAGIC) & MASK, 42)
        emmc_cid = rng.randbytes(16)
        plaintext = rng.randbytes(510) + b"\x55\xaa"
        offset = case * 0x877 * 32  # AES block offset, as used by nandio.c
        ctr = int.from_bytes(hashlib.sha1(emmc_cid).digest()[:16], "little")
        aes = Cipher(algorithms.AES(normal_key.to_bytes(16, "big")), modes.ECB()).encryptor()
        ciphertext = bytearray()
        for block in range(32):
            counter = ((ctr + offset + block) & MASK).to_bytes(16, "big")
            stream = aes.update(counter)[::-1]
            source = plaintext[block * 16:(block + 1) * 16]
            ciphertext.extend(a ^ b for a, b in zip(source, stream))

        arm.put_int(ARGS, normal_key)
        arm.call("F_XY_reverse", ARGS, ARGS + 32)
        recovered_x = (arm.get_int(ARGS + 32) ^ KEY_Y).to_bytes(16, "little")
        recovered_id = (recovered_x[:4] + recovered_x[12:])[::-1]
        assert recovered_id == console_id.to_bytes(8, "big"), "Console ID recovery failed"

        for identity in (console_id.to_bytes(8, "big"), recovered_id):
            arm.cpu.mem_write(ARGS, identity)
            arm.cpu.mem_write(ARGS + 32, emmc_cid)
            arm.call("dsi_crypt_init", ARGS, ARGS + 32, 0)
            arm.cpu.mem_write(ARGS + 64, bytes(ciphertext))
            arm.call("dsi_nand_crypt", ARGS + 64, ARGS + 64, offset, 32)
            assert bytes(arm.cpu.mem_read(ARGS + 64, 512)) == plaintext, (
                f"NAND sector {case} differs from independent AES/SHA1 reference"
            )
    print("PASS: 32 NAND sector checks including recovered console IDs, AES and SHA1")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--elf", type=Path, default=ROOT / "arm9/akmenu-next.elf")
    parser.add_argument("--group", choices=("keys", "nand", "all"), default="all")
    options = parser.parse_args()
    arm = Arm9(options.elf)
    if options.group in ("keys", "all"):
        check_keys(arm)
    if options.group in ("nand", "all"):
        check_nand(arm)
