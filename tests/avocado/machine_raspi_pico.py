# Functional tests for the Raspberry Pico Platform
#
# Copyright (c) 2022 Mateusz Stadnik <matgla@live.com>
#
# This work is licensed under the terms of the GNU GPL, version 2 or later.
# See the COPYING file in the top-level directory.
#
# SPDX-License-Identifier: GPL-2.0-or-later

from pathlib import Path

from avocado_qemu import QemuSystemTest
from avocado_qemu import wait_for_console_pattern

RASPI_PICO_FLASH_SIZE = 2 * 1024 * 1024

def create_spi_flash_image(original_file, target_file):
    with open(original_file, 'r+b') as original:
        data = bytearray(original.read())
        data = data + bytearray(RASPI_PICO_FLASH_SIZE - len(data))
        with open(target_file, "w+b") as output:
            output.write(data)


class RaspiPicoMachine(QemuSystemTest):
    timeout = 5
    def test_serial_port(self):
        """
        :avocado: tags=arch:arm
        :avocado: tags=machine:raspi_pico
        """
        """
        https://github.com/matgla/qemu_raspberry_pico_tests/raw/main/hello_world/serial/hello_serial.bin
        constantly prints out 'hello world'
        """
        rom_url = ('https://github.com/matgla/qemu_raspberry_pico_tests'
                   '/raw/main/hello_world/serial/hello_serial.bin')
        rom_hash = "b1e9ff03e92320553d653d4f1de5375a325c44f9"
        rom_path = self.fetch_asset(rom_url, asset_hash=rom_hash)
        parent = Path(rom_path).parent
        flash_path = Path.joinpath(parent, "raspi_pico_flash.bin")

        create_spi_flash_image(rom_path, flash_path.absolute())
        self.vm.add_args('-drive if=mtd,format=raw,file=' + str(flash_path.absolute()))
        self.vm.add_args('-nographic')
        self.vm.launch()

        wait_for_console_pattern("Hello, world!")
        self.vm.shutdown()