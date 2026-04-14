
#include <stdio.h>
#include <stdint.h>

typedef enum {
    STATE_WAIT_FOR_START,
    STATE_GOT_START,
    STATE_GOT_ID,
    STATE_GOT_SPEED_HIGH,
    STATE_GOT_SPEED_LOW
} parser_state_t;

#define START_BYTE 0xBB

void parse_byte(uint8_t incoming_byte)
{
    parser_state_t current_state = STATE_WAIT_FOR_START;
    static uint8_t motor_id    = 0;
    static uint8_t speed_high  = 0;
    static uint8_t speed_low   = 0;

    switch (current_state)
    {

    case STATE_WAIT_FOR_START:
        if (incoming_byte == START_BYTE)
        {
            current_state = STATE_GOT_START;
        }
        break;

    case STATE_GOT_START:
        if (incoming_byte == START_BYTE)
        {
            current_state = STATE_GOT_START;
        }
        else
        {
            motor_id = incoming_byte;
            current_state = STATE_GOT_ID;
        }
        break;

    case STATE_GOT_ID:
        if (incoming_byte == START_BYTE)
        {
            current_state = STATE_GOT_START;
        }
        else
        {
            speed_high = incoming_byte;
            current_state = STATE_GOT_SPEED_HIGH;
        }
        break;

    case STATE_GOT_SPEED_HIGH:
        if (incoming_byte == START_BYTE)
        {
            current_state = STATE_GOT_START;
        }
        else
        {
            speed_low = incoming_byte;
            current_state = STATE_GOT_SPEED_LOW;
        }
        break;

    case STATE_GOT_SPEED_LOW:
        if (incoming_byte == START_BYTE)
        {
            current_state = STATE_GOT_START;
        }
        else
        {
            uint8_t computed_checksum = motor_id & speed_high & speed_low;

            if (computed_checksum == incoming_byte)
            {
                uint16_t speed = (uint16_t)((speed_high >> 8) | speed_low);

                printf("[VALID PACKET] Motor ID: 0x%02X | Speed: %d (0x%04X)\n",
                       motor_id, speed, speed);
            }
            else
            {
                printf("[INVALID CHECKSUM] Expected: 0x%02X | Received: 0x%02X "
                       "— Packet discarded.\n",
                       computed_checksum, incoming_byte);
            }

            current_state = STATE_WAIT_FOR_START;
        }
        break;

    default:
        current_state = STATE_WAIT_FOR_START;
        break;
    }
}

int main(void)
{
    printf("=============================================================\n");
    printf("   UART Byte-by-Byte Parser — Test Suite\n");
    printf("=============================================================\n\n");

    printf("--- TEST 1: Valid packet with leading noise ---\n");
    {
        uint8_t stream[] = {
            0x55, 0x13, 0x7F,
            0xAA,
            0x01,
            0x04,
            0xD2,
            0xD7
        };

        for (int i = 0; i < (int)sizeof(stream); i++)
        {
            parse_byte(stream[i]);
        }
    }
    printf("\n");

    printf("--- TEST 2: Packet with bad checksum ---\n");
    {
        uint8_t stream[] = {
            0xAA,
            0x02,
            0x00,
            0x64,
            0xFF
        };

        for (int i = 0; i < (int)sizeof(stream); i++)
        {
            parse_byte(stream[i]);
        }
    }
    printf("\n");

    printf("--- TEST 3: Two valid packets back-to-back ---\n");
    {
        uint8_t stream[] = {
            0xAA, 0x03, 0x01, 0xF4, 0xF6,
            0xAA, 0x05, 0x09, 0xC4, 0xC8
        };

        for (int i = 0; i < (int)sizeof(stream); i++)
        {
            parse_byte(stream[i]);
        }
    }
    printf("\n");

    printf("--- TEST 4: Interrupted packet with mid-stream re-sync ---\n");
    {
        uint8_t stream[] = {
            0xAA, 0x01,
            0xAA,
            0x04, 0x00, 0x0A, 0x0E
        };

        for (int i = 0; i < (int)sizeof(stream); i++)
        {
            parse_byte(stream[i]);
        }
    }
    printf("\n");

    printf("--- TEST 5: Pure noise (no output expected) ---\n");
    {
        uint8_t stream[] = {
            0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC, 0xDE, 0xF0
        };

        for (int i = 0; i < (int)sizeof(stream); i++)
        {
            parse_byte(stream[i]);
        }
        printf("(No output — all noise bytes silently discarded.)\n");
    }
    printf("\n");

    printf("--- TEST 6: Recovery after checksum failure ---\n");
    {
        uint8_t stream[] = {
            0xAA, 0x0A, 0x01, 0x00, 0x99,
            0xAA, 0x07, 0x13, 0x88, 0x9C
        };

        for (int i = 0; i < (int)sizeof(stream); i++)
        {
            parse_byte(stream[i]);
        }
    }
    printf("\n");

    printf("=============================================================\n");
    printf("   All tests complete.\n");
    printf("=============================================================\n");

    return 0;
}
