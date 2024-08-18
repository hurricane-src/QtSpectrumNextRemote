#pragma once
#include <cstdint>
#include <QByteArray>

class NEX
{
public:
    static const uint32_t MAGIC = 0x7478654E;
    static const uint32_t HEADER_SIZE = 512;
    static const uint32_t BANK_COUNT = 112;
    static const uint32_t FIRST_BANK_INDEXES_COUNT = 6;
protected:

    static const uint8_t FIRST_BANK_INDEXES[FIRST_BANK_INDEXES_COUNT];

    uint8_t* _header;
    uint8_t* _palette;
    uint8_t* _layer2;
    uint8_t* _ula;
    uint8_t* _lores;
    uint8_t* _hires;
    uint8_t* _hicolour;
    uint8_t** _banks;

public:
    NEX();
    bool load(const QString& filename);
    uint8_t *getPage(uint8_t page);

private:
    uint8_t loadingScreenBlocksInFile();
    uint8_t getPageAtIndex(int index);
    bool isBankPresent(int page);
    bool hasPalette();
    bool hasLayer2();
    bool hasULA();
    bool hasLoRes();
    bool hasHiRes();
    bool hasHiColour();
    uint8_t entry16kbank();
public:
    uint16_t SP();
    uint16_t PC();
    bool uses16KBank(uint8_t bank);
    bool uses8KBank(uint8_t bank);
};
