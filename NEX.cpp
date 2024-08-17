//
// Created by ediazfer on 14/08/24.
//

#include "NEX.h"
#include <QFile>

const uint8_t NEX::FIRST_BANK_INDEXES[FIRST_BANK_INDEXES_COUNT] = { 5, 2, 0, 1, 3, 4};

NEX::NEX():_palette(nullptr),_layer2(nullptr),_ula(nullptr),
      _lores(nullptr),_hires(nullptr),_hicolour(nullptr)
{
    _header = new uint8_t[HEADER_SIZE];
    _banks = new uint8_t*[BANK_COUNT];
}

uint8_t NEX::LoadingScreenBlocksInFile()
{
    return _header[10];
}

uint8_t NEX::GetPageAtIndex(int index)
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

bool NEX::IsBankPresent(int page)
{
    return _header[18 + page] != 0;
}

bool NEX::HasPalette()
{
    uint8_t blocks = LoadingScreenBlocksInFile();

    if((blocks & 5) != 0) // wtf is the tilemap mode?
    {
        return (blocks & 128) == 0;
    }
    else
    {
        return false;
    }
}

bool NEX::HasLayer2() { return (LoadingScreenBlocksInFile() & 1) != 0; }
bool NEX::HasULA() { return (LoadingScreenBlocksInFile() & 2) != 0; }
bool NEX::HasLoRes() { return (LoadingScreenBlocksInFile() & 4) != 0; }
bool NEX::HasHiRes() { return (LoadingScreenBlocksInFile() & 8) != 0; }
bool NEX::HasHiColour() { return (LoadingScreenBlocksInFile() & 16) != 0; }

uint16_t NEX::SP() { return *((uint16_t*)&_header[12]); }
uint16_t NEX::PC() { return *((uint16_t*)&_header[14]); }
uint8_t NEX::Entry16kbank() { return (uint8_t)_header[139]; }

bool NEX::Uses16KBank(uint8_t bank)
{
    if(/*bank >= 0 && */bank < 112)
    {
        return IsBankPresent(bank);
    }
    return false;
}

bool NEX::Uses8KBank(uint8_t bank)
{
    bank >>= 1;
    if(/*bank >= 0 && */bank < 112)
    {
        return IsBankPresent(bank);
    }
    return false;
}

bool NEX::Load(const QString& filename)
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
    if(HasPalette())
    {
        _palette = new uint8_t[512];
        fs.read((char*)_palette, 512);
    }
    else
    {
        _palette = nullptr;
    }

    if(HasLayer2())
    {
        _layer2 = new uint8_t[49152];
        fs.read((char*)_layer2, 49152);
    }
    else
    {
        _layer2 = nullptr;
    }

    if(HasULA())
    {
        _ula = new uint8_t[6912];
        fs.read((char*)_ula, 6912);
    }
    else
    {
        _ula = nullptr;
    }

    if(HasLoRes())
    {
        _lores = new uint8_t[12288];
        fs.read((char*)_lores, 12288);
    }
    else
    {
        _lores = nullptr;
    }

    if(HasHiRes())
    {
        _hires = new uint8_t[12288];
        fs.read((char*)_hires, 12288);
    }
    else
    {
        _hires = nullptr;
    }

    if(HasHiColour())
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
        int bank = GetPageAtIndex(i);
        if(IsBankPresent(bank))
        {
            _banks[bank] = new uint8_t[16384];
            fs.read((char*)_banks[bank], 16384);
        }
    }

    return true;
}

uint8_t* NEX::GetPage(uint8_t page)
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
