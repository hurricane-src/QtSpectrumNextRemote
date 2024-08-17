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
    bool Load(const QString& filename);
    uint8_t *GetPage(uint8_t page);

private:
    uint8_t LoadingScreenBlocksInFile();
    uint8_t GetPageAtIndex(int index);
    bool IsBankPresent(int page);
    bool HasPalette();
    bool HasLayer2();
    bool HasULA();
    bool HasLoRes();
    bool HasHiRes();
    bool HasHiColour();
    uint8_t Entry16kbank();
public:
    uint16_t SP();
    uint16_t PC();
    bool Uses16KBank(uint8_t bank);
    bool Uses8KBank(uint8_t bank);
};
