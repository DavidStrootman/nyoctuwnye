//
// Created by David Strootman on 9-4-2019.
//
// Most of the comments in this file come from Cowgod's Chip-8 Technical reference
//

#include <thread>
#include "include/chip8.h"

chip8::chip8(): I(), sp(), delay_timer(), sound_timer(), draw_flag(), gfx(), V()
{
    pc = 0x200; // Program counter (First 512/0x200 bytes are reserved for Chip8)
    opcode = 0; // Current opcode
}

void chip8::initialize()
{
    // zero-initialize gfx, stack, registers and memory
    memset(gfx, 0, sizeof(gfx));
    memset(stack, 0, sizeof(stack));
    memset(V, 0, sizeof(V));
    memset(memory, 0, sizeof(memory));

    for (int i = 0; i < 80; ++i) {
        memory[i] = chip8_fontset[i];
    }

    I = 0; // Index Register
    sp = 0; // Stack pointer

    draw_flag = false;

    delay_timer = 0;
    sound_timer = 0;

    timer_loop();
}

void chip8::timer_loop()
{
    for(;;) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1000 / 60));
        if (delay_timer > 0)
            --delay_timer;
        if (sound_timer > 0)
            --sound_timer;
    }
}

void chip8::emulateCycle()
{
    opcode = memory[pc] << 8 | memory[pc + 1];
    char reg1 = 0x00;

    // Decode opcode
    switch (opcode & 0xF000) {
        case 0x0000: {
            switch (opcode & 0x000F) {
                case 0x0000: {
                    /*
                     * 00E0 - CLS
                     * Clear the display.
                     */
                    memset(gfx, 0, sizeof(gfx));
                    break;
                }

                case 0x000E: {

                    /*
                     * 00EE - RET
                     * Return from a subroutine.
                     *
                     * The interpreter sets the program counter to the address at the top of the stack,
                     * then subtracts 1 from the stack pointer.
                     */
                    pc = stack[sp--] + 1;
                    break;
                }
                default: {
                    goto unknown_opcode;
                }
            }
            break;
        }

        case 0x1000: {
            /*
             * 1nnn - JP addr
             * Jump to location nnn.
             * The interpreter sets the program counter to nnn.
             */
            pc = NNN;
            break;
        }

        case 0x2000: {
            /*
             * 2nnn - CALL addr
             * Call subroutine at nnn.
             *
             * The interpreter increments the stack pointer, then puts the current PC on the top of the stack.
             * The PC is then set to nnn.
             */
            stack[sp] = pc;
            ++sp;
            pc = NNN;
            break;
        }

        case 0x3000: {
            /*
             * 3xkk - SE Vx, byte
             * Skip next instruction if Vx = kk.
             *
             * The interpreter compares register Vx to kk, and if they are equal, increments the program counter by 2.
             */

            if (X == KK) {
                pc += 2;
            }
            pc += 2;
            break;
        }

        case 0x4000: {
            /*
             * 4xkk - SNE Vx, byte
             * Skip next instruction if Vx != kk.
             *
             * The interpreter compares register Vx to kk, and if they are not equal, increments the program counter by 2.
             */

            if (X != KK) {
                pc += 2;
            }
            pc += 2;
            break;
        }

        case 0x5000: {
            /*
             * 5xy0 - SE Vx, Vy
             * Skip next instruction if Vx = Vy.
             *
             * The interpreter compares register Vx to register Vy, and if they are equal,
             * increments the program counter by 2.
             */

            if (X == Y) {
                pc += 2;
            }
            pc += 2;
            break;
        }

        case 0x6000: {
            /*
             * 6xkk - LD Vx, byte
             * Set Vx = kk.
             *
             * The interpreter puts the value kk into register Vx.
             */
            V[X] = KK;

            pc += 2;
            break;
        }

        case 0x7000: {
            /*
             * 7xkk - ADD Vx, byte
             * Set Vx = Vx + kk.
             *
             * Adds the value kk to the value of register Vx, then stores the result in Vx.
             */
            V[X] += KK;

            pc += 2;
            break;
        }

        case 0x8000: {
            switch (opcode & 0x000F) {
                case 0x0000: {
                    /*
                     * 8xy0 - LD Vx, Vy
                     * Set Vx = Vy.
                     *
                     * Stores the value of register Vy in register Vx.
                     */
                    V[X] = V[Y];

                    pc += 2;
                    break;
                }

                case 0x0001: {
                    /*
                     * 8xy1 - OR Vx, Vy
                     * Set Vx = Vx OR Vy.
                     *
                     * Performs a bitwise OR on the values of Vx and Vy, then stores the result in Vx.
                     * A bitwise OR compares the corresponding bits from two values, and if either bit is 1,
                     * then the same bit in the result is also 1. Otherwise, it is 0.
                     */
                    V[X] |= V[Y];

                    pc += 2;
                    break;
                }

                case 0x0002: {
                    /*
                     * 8xy2 - AND Vx, Vy
                     * Set Vx = Vx AND Vy.
                     *
                     * Performs a bitwise AND on the values of Vx and Vy, then stores the result in Vx.
                     * A bitwise AND compares the corresponding bits from two values, and if both bits are 1,
                     * then the same bit in the result is also 1. Otherwise, it is 0.
                     */
                    V[X] &= V[Y];

                    pc += 2;
                    break;
                }

                case 0x0003: {
                    /*
                     * 8xy3 - XOR Vx, Vy
                     * Set Vx = Vx XOR Vy.
                     *
                     * Performs a bitwise exclusive OR on the values of Vx and Vy, then stores the result in Vx.
                     * An exclusive OR compares the corresponding bits from two values,
                     * and if the bits are not both the same, then the corresponding bit in the result is set to 1.
                     * Otherwise, it is 0.
                     */
                    V[X] ^= V[Y];

                    pc += 2;
                    break;
                }

                case 0x0004: {
                    /*
                     * 8xy4 - ADD Vx, Vy
                     * Set Vx = Vx + Vy, set VF = carry.
                     *
                     * The values of Vx and Vy are added together.
                     * If the result is greater than 8 bits (i.e., > 255,) VF is set to 1, otherwise 0.
                     * Only the lowest 8 bits of the result are kept, and stored in Vx.
                     */
                    if ((V[X] += V[Y]) > 255) {
                        V[0xF] = 1;
                    } else {
                        V[0xF] = 0;
                    }

                    V[X] &= 0x00FF;

                    pc += 2;
                    break;
                }

                case 0x0005: {
                    /*
                     * 8xy5 - SUB Vx, Vy
                     * Set Vx = Vx - Vy, set VF = NOT borrow.
                     *
                     * If Vx > Vy, then VF is set to 1, otherwise 0.
                     * Then Vy is subtracted from Vx, and the results stored in Vx.
                     */
                    V[X] -= V[Y];
                    V[0xF] = V[X] > V[Y];

                    pc += 2;
                    break;
                }

                case 0x0006: {
                    /*
                     * 8xy6 - SHR Vx {, Vy}
                     * Set Vx = Vx SHR 1.
                     *
                     * If the least-significant bit of Vx is 1, then VF is set to 1, otherwise 0.
                     * Then Vx is divided by 2.
                     */
                    V[0xF] = V[X] & 1;
                    V[X] >>= 1;

                    pc += 2;
                    break;
                }

                case 0x0007: {
                    /*
                     * 8xy7 - SUBN Vx, Vy
                     * Set Vx = Vy - Vx, set VF = NOT borrow.
                     *
                     * If Vy > Vx, then VF is set to 1, otherwise 0.
                     * Then Vx is subtracted from Vy, and the results stored in Vx.
                     */
                    V[0xF] = V[Y] > V[X];
                    V[X] = V[Y] - V[X];

                    pc += 2;
                    break;
                }

                case 0x000E: {
                    /*
                     * 8xyE - SHL Vx {, Vy}
                     * Set Vx = Vx SHL 1.
                     *
                     * If the most-significant bit of Vx is 1, then VF is set to 1, otherwise to 0.
                     * Then Vx is multiplied by 2.
                     */
                    V[X] <<= 1;
                    V[0xF] = V[X] & 0x80;

                    pc += 2;
                    break;
                }

                default: {
                    goto unknown_opcode;
                }
            }
            break;
        }

        case 0x9000: {
            /*
             * 9xy0 - SNE Vx, Vy
             * Skip next instruction if Vx != Vy.
             *
             * The values of Vx and Vy are compared, and if they are not equal, the program counter is increased by 2.
             */
            pc += (V[X] != V[Y]) ? 2 : 0;

            pc += 2;
            break;
        }

        case 0xA000: {
            /*
             * Annn - LD I, addr
             * Set I = nnn.
             *
             * The value of register I is set to nnn.
             */
            I = opcode & 0x0FFF;

            pc += 2;
            break;
        }

        case 0xB000: {
            /*
             * Bnnn - JP V0, addr
             * Jump to location nnn + V0.
             *
             * The program counter is set to nnn plus the value of V0.
             */
            pc = NNN + V[0x0];

            pc += 2;
            break;
        }

        case 0xC000: {
            /*
             * Cxkk - RND Vx, byte
             * Set Vx = random byte AND kk.
             *
             * The interpreter generates a random number from 0 to 255, which is then ANDed with the value kk.
             * The results are stored in Vx. See instruction 8xy2 for more information on AND.
             */

            auto randVal = (unsigned char) rand();
            V[X] = randVal & KK;

            pc += 2;
            break;
        }

        case 0xD000: {
            /*
             * Dxyn - DRW Vx, Vy, nibble
             * Display n-byte sprite starting at memory location I at (Vx, Vy), set VF = collision.
             *
             * The interpreter reads n bytes from memory, starting at the address stored in I. These bytes are
             * then displayed as sprites on screen at coordinates (Vx, Vy). Sprites are XORed onto the existing screen.
             * If this causes any pixels to be erased, VF is set to 1, otherwise it is set to 0. If the sprite is
             * positioned so part of it is outside the coordinates of the display, it wraps around to the opposite
             * side of the screen. See instruction 8xy3 for more information on XOR, and section 2.4, Display,
             * for more information on the Chip-8 screen and sprites.
             */
            unsigned short height = opcode & 0x000F;
            unsigned short pixel;

            V[0xF] = 0;
            for (int yline = 0; yline < height; yline++) {
                pixel = memory[I + yline];
                for (int x_line = 0; x_line < 8; x_line++) {
                    if ((pixel & (0x80 >> x_line)) != 0) {
                        if (gfx[(X + x_line + ((Y + yline) * 64))] == 1)
                            V[0xF] = 1;
                        gfx[X + x_line + ((Y + yline) * 64)] ^= 1;
                    }
                }
            }

            draw_flag = true;

            pc += 2;
            break;
        }

        case 0xE000: {
            switch (opcode & 0x000F) {
                case 0x000E: {
                    /*
                     * Ex9E - SKP Vx
                     * Skip next instruction if key with the value of Vx is pressed.
                     *
                     * Checks the keyboard, and if the key corresponding to the value of Vx is currently in
                     * the down position, PC is increased by 2.
                     */
                    // Windows key state
                    if (GetKeyState(V[X]) & 0x8000) {
                        pc += 2;
                    }

                    pc += 2;
                    break;
                }

                case 0x0001: {
                    /*
                     * ExA1 - SKNP Vx
                     * Skip next instruction if key with the value of Vx is not pressed.
                     *
                     * Checks the keyboard, and if the key corresponding to the value of Vx is currently in
                     * the up position, PC is increased by 2.
                     */
                    if (!(GetKeyState(V[X]) & 0x8000)) {
                        pc += 2;
                    }

                    pc += 2;
                    break;
                }

                default: {
                    goto unknown_opcode;
                }
            }
            break;
        }

        case 0xF000: {
            switch (opcode & 0x00F0) {
                case 0x0000: {
                    switch (opcode & 0x000F) {
                        case 0x0007: {
                            /*
                             * Fx07 - LD Vx, DT
                             * Set Vx = delay timer value.
                             *
                             * The value of DT is placed into Vx.
                             */
                            V[X] = delay_timer;

                            break;
                        }

                        case 0x000A: {
                            // TODO: check if correct character is given
                            /*
                             * Fx0A - LD Vx, K
                             * Wait for a key press, store the value of the key in Vx.
                             *
                             * All execution stops until a key is pressed, then the value of that key is stored in Vx.
                            */
                            unsigned char key;
                            key = getch();
                            V[X] = key;

                            pc += 2;
                            break;
                        }

                        default: {
                            goto unknown_opcode;
                        }
                    }
                }

                case 0x0010: {
                    switch (opcode & 0x000F) {
                        case 0x0005: {
                            /*
                             * Fx15 - LD DT, Vx
                             * Set delay timer = Vx.
                             *
                             * DT is set equal to the value of Vx.
                             */
                            delay_timer = V[X];

                            pc += 2;
                            break;
                        }

                        case 0x0008: {
                            /*
                             * Fx18 - LD ST, Vx
                             * Set sound timer = Vx.
                             *
                             * ST is set equal to the value of Vx.
                             */
                            sound_timer = V[X];

                            pc += 2;
                            break;
                        }

                        case 0x000E: {
                            /*
                             * Fx1E - ADD I, Vx
                             * Set I = I + Vx.
                             *
                             * The values of I and Vx are added, and the results are stored in I.
                             */
                            I += V[X];

                            pc += 2;
                            break;
                        }

                        default: {
                            goto unknown_opcode;
                        }
                    }
                }

                case 0x0020: {
                    // TODO: Make sure this works
                    /*
                     * Fx29 - LD F, Vx
                     * Set I = location of sprite for digit Vx.
                     *
                     * The value of I is set to the location for the hexadecimal sprite corresponding to
                     * the value of Vx. See section 2.4, Display, for more information on the Chip-8 hexadecimal font.
                     */
                    I = memory[V[X] * 5];

                    pc += 2;
                    break;
                }

                case 0x0030: {
                    /*
                     * Fx33 - LD B, Vx
                     * Store BCD representation of Vx in memory locations I, I+1, and I+2.
                     *
                     * The interpreter takes the decimal value of Vx, and places the hundreds digit in memory at
                     * location in I, the tens digit at location I+1, and the ones digit at location I+2.
                     */
                    memory[I] = V[X] % 10;
                    memory[I + 1] = round((V[X] / 10) % 10);
                    memory[I + 2] = round((V[X] / 100) % 10);

                    break;
                }

                case 0x0050: {
                    /*
                     * Fx55 - LD [I], Vx
                     * Store registers V0 through Vx in memory starting at location I.
                     *
                     * The interpreter copies the values of registers V0 through Vx into memory,
                     * starting at the address in I.
                     */
                    for (unsigned int i = 0; i < X; ++i) {
                        memory[I + i] = V[0 + i];
                    }

                    pc += 2;
                    break;
                }

                case 0x0060: {
                    /*
                     * Fx65 - LD Vx, [I]
                     * Read registers V0 through Vx from memory starting at location I.
                     *
                     * The interpreter reads values from memory starting at location I into registers V0 through Vx.
                     */

                    for (int i = 0; i < X; ++i) {
                        V[i] = memory[I + i];
                    }

                    pc += 2;
                    break;
                }

                default: {
                    goto unknown_opcode;
                }
            }
        }

        default: {
        unknown_opcode:
            std::cout << "Unknown opcode: " << opcode << "\n";
        }
    }
}