#define  RST      8 // transistor base for ~RESET
#define  SCI      9 // PB3
#define  SDO     10 // PB2
#define  SII     11 // PB1
#define  SDI     12 // PB0
#define  VCC     13 // VCC

enum command_id {
    CMD_CHIP_ERASE,
    CMD_NOOP,
    CMD_READ_FUSE_LOW,
    CMD_READ_FUSE_HIGH,
    CMD_READ_LOCK_BITS,
    CMD_ARGS_0,
    CMD_WRITE_FUSE_LOW,
    CMD_WRITE_FUSE_HIGH,
    CMD_WRITE_LOCK_BITS,
    CMD_READ_SIGNATURE,
    CMD_ARGS_2ND,
};

struct command {
    uint8_t len;
    uint8_t sdi[6];
    uint8_t sii[6];
};

static const struct command commands[] = {
    { // CMD_CHIP_ERASE
        len: 3,
        sdi: {0x80, 0x00, 0x00},
        sii: {0x4C, 0x64, 0x6C},
    },
    { // CMD_NOOP
        len: 1,
        sdi: {0x00},
        sii: {0x4c},
    },
    { // CMD_READ_FUSE_LOW
        len: 3,
        sdi: {0x04, 0x00, 0x00},
        sii: {0x4C, 0x68, 0x6C},
    },
    { // CMD_READ_FUSE_HIGH
        len: 3,
        sdi: {0x04, 0x00, 0x00},
        sii: {0x4C, 0x7A, 0x7E},
    },
    { // CMD_READ_LOCK_BITS
        len: 3,
        sdi: {0x04, 0x00, 0x00},
        sii: {0x4C, 0x78, 0x7C},
    },
    { 0 },
    { // CMD_WRITE_FUSE_LOW
        len: 4,
        sdi: {0x40, 0x00, 0x00, 0x00},
        sii: {0x4C, 0x2C, 0x64, 0x6C},
    },
    { // CMD_WRITE_FUSE_HIGH
        len: 4,
        sdi: {0x40, 0x00, 0x00, 0x00},
        sii: {0x4C, 0x2C, 0x74, 0x7C},
    },
    { // CMD_WRITE_LOCK_BITS
        len: 4,
        sdi: {0x20, 0x00, 0x00, 0x00},
        sii: {0x4C, 0x2C, 0x64, 0x6C},
    },
    { // CMD_READ_SIGNATURE
        len: 4,
        sdi: {0x08, 0x00, 0x00, 0x00},
        sii: {0x4C, 0x0C, 0x68, 0x6C},
    },
    { 0 },
};

uint8_t _write_cmd(const struct command *cmd)
{
    uint16_t ret = 0;

    // Wait for SDO to go high
    while (!digitalRead(SDO))
        ;

    for (uint8_t i = 0; i < cmd->len; i++) {
        uint16_t sdi = (cmd->sdi[i]) << 2;
        uint16_t sii = (cmd->sii[i]) << 2;

        for (int8_t j = 10; j >= 0; j--) {
            digitalWrite(SDI, !!(sdi & (1 << j)));
            digitalWrite(SII, !!(sii & (1 << j)));
            if (i == cmd->len-1) {
                ret <<= 1;
                ret |= digitalRead(SDO);
            }
            digitalWrite(SCI, HIGH);
            digitalWrite(SCI, LOW);
        }
    }

    return ret >> 2;
}

uint8_t _command(enum command_id id, uint8_t val)
{
    if (id < CMD_ARGS_0) {
        return _write_cmd(&commands[id]);
    } else if (id > CMD_ARGS_0 && id < CMD_ARGS_2ND) {
        struct command tmp = commands[id];
        tmp.sdi[1] = val;
        return _write_cmd(&tmp);
    }

    return 0;
}

void cmd_chip_erase(void)
{
    _command(CMD_CHIP_ERASE, 0);
    _command(CMD_NOOP, 0);
}

void cmd_set_fuses(uint8_t lfuse, uint8_t hfuse)
{
    _command(CMD_WRITE_FUSE_LOW, lfuse);
    _command(CMD_WRITE_FUSE_HIGH, hfuse);
}

void print_fuses(void)
{
    uint8_t  ret;

    ret = _command(CMD_READ_FUSE_LOW, 0);
    Serial.print("LFUSE: ");
    Serial.print(ret, HEX);

    ret = _command(CMD_READ_FUSE_HIGH, 0);
    Serial.print(" HFUSE: ");
    Serial.print(ret, HEX);

    ret = _command(CMD_READ_LOCK_BITS, 0);
    Serial.print(" LOCK: ");
    Serial.println(ret, HEX);
}

uint16_t read_signature(void)
{
    uint16_t ret;

    ret = _command(CMD_READ_SIGNATURE, 1) << 8;
    ret |= _command(CMD_READ_SIGNATURE, 2);

    return ret;
}


void prepare_hv_mode(void)
{
    pinMode(SDO, OUTPUT);
    digitalWrite(SDI, LOW);
    digitalWrite(SDO, LOW);
    digitalWrite(SII, LOW);
    digitalWrite(RST, HIGH);
    digitalWrite(VCC, HIGH);

    delayMicroseconds(20);

    digitalWrite(RST, LOW);

    delayMicroseconds(10);

    pinMode(SDO, INPUT);
    delayMicroseconds(300);
}

void setup() {
    pinMode(RST, OUTPUT);
    pinMode(SCI, OUTPUT);
    pinMode(SDO, OUTPUT);
    pinMode(SII, OUTPUT);
    pinMode(SDI, OUTPUT);
    pinMode(VCC, OUTPUT);
    digitalWrite(RST, HIGH); // disable

    Serial.begin(9600);
    while (!Serial) {
    }

    Serial.println("START");
}

void loop() {
    Serial.println();
    Serial.print("Waiting for a key...");
    Serial.flush();
    while(!Serial.available()) ;
    while(Serial.available()) Serial.read();

    Serial.println("B");

    prepare_hv_mode();

    uint16_t sig = read_signature();
    Serial.print("Signature: ");
    Serial.println(sig, HEX);

    print_fuses();

    Serial.println(" * Chip erase..");
    cmd_chip_erase();

    Serial.println(" * Reset fuses..");
    cmd_set_fuses(0x6A, 0xFF);

    print_fuses();
}
