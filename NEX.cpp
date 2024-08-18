#include "NEX.h"
#include <QFile>

const uint8_t NEX::FIRST_BANK_INDEXES[FIRST_BANK_INDEXES_COUNT] = { 5, 2, 0, 1, 3, 4};

NEX::NEX():_palette(nullptr),_layer2(nullptr),_ula(nullptr),
      _lores(nullptr),_hires(nullptr),_hicolour(nullptr)
{
    _header = new uint8_t[HEADER_SIZE];
    _banks = new uint8_t*[BANK_COUNT];
}

uint8_t NEX::loadingScreenBlocksInFile()
{
    return _header[10];
}

uint8_t NEX::getPageAtIndex(int index)
{
    if(index < FIRST_BANK_INDEXES_COUNT)
    {
        return FIRST_BANK_INDEXES[index];
    }
    else
    {
        return index;
    }
}

bool NEX::isBankPresent(int page)
{
    return _header[18 + page] != 0;
}

bool NEX::hasPalette()
{
    uint8_t blocks = loadingScreenBlocksInFile();

    if((blocks & 5) != 0) // wtf is the tilemap mode?
    {
        return (blocks & 128) == 0;
    }
    else
    {
        return false;
    }
}

bool NEX::hasLayer2() { return (loadingScreenBlocksInFile() & 1) != 0; }
bool NEX::hasULA() { return (loadingScreenBlocksInFile() & 2) != 0; }
bool NEX::hasLoRes() { return (loadingScreenBlocksInFile() & 4) != 0; }
bool NEX::hasHiRes() { return (loadingScreenBlocksInFile() & 8) != 0; }
bool NEX::hasHiColour() { return (loadingScreenBlocksInFile() & 16) != 0; }

uint16_t NEX::SP() { return *((uint16_t*)&_header[12]); }
uint16_t NEX::PC() { return *((uint16_t*)&_header[14]); }
uint8_t NEX::entry16kbank() { return (uint8_t)_header[139]; }

bool NEX::uses16KBank(uint8_t bank)
{
    if(/*bank >= 0 && */bank < 112)
    {
        return isBankPresent(bank);
    }
    return false;
}

bool NEX::uses8KBank(uint8_t bank)
{
    bank >>= 1;
    if(/*bank >= 0 && */bank < 112)
    {
        return isBankPresent(bank);
    }
    return false;
}

bool NEX::load(const QString& filename)
{
    for(int i = 0; i < BANK_COUNT; ++i)
    {
        _banks[i] = nullptr;
    }

    QFile fs(filename);
    if(!fs.open(QIODevice::ReadOnly))
    {
        return false;
    }

    fs.read((char*)_header, HEADER_SIZE);
    uint32_t *magicp = (uint32_t*)&_header[0];
    uint magic = *magicp;
    if(magic != MAGIC)
    {
        //throw new FormatException();
        return false;
    }
    if(hasPalette())
    {
        _palette = new uint8_t[512];
        fs.read((char*)_palette, 512);
    }
    else
    {
        _palette = nullptr;
    }

    if(hasLayer2())
    {
        _layer2 = new uint8_t[49152];
        fs.read((char*)_layer2, 49152);
    }
    else
    {
        _layer2 = nullptr;
    }

    if(hasULA())
    {
        _ula = new uint8_t[6912];
        fs.read((char*)_ula, 6912);
    }
    else
    {
        _ula = nullptr;
    }

    if(hasLoRes())
    {
        _lores = new uint8_t[12288];
        fs.read((char*)_lores, 12288);
    }
    else
    {
        _lores = nullptr;
    }

    if(hasHiRes())
    {
        _hires = new uint8_t[12288];
        fs.read((char*)_hires, 12288);
    }
    else
    {
        _hires = nullptr;
    }

    if(hasHiColour())
    {
        _hicolour = new uint8_t[12288];
        fs.read((char*)_hicolour, 12288);
    }
    else
    {
        _hicolour = nullptr;
    }

    // no clue

    for(int i = 0; i < 112; ++i)
    {
        int bank = getPageAtIndex(i);
        if(isBankPresent(bank))
        {
            _banks[bank] = new uint8_t[16384];
            fs.read((char*)_banks[bank], 16384);
        }
    }

    return true;
}

uint8_t* NEX::getPage(uint8_t page)
{
    if(/*page >= 0 && */page < BANK_COUNT)
    {
        return _banks[page];
    }
    else
    {
        return nullptr;
    }
}
