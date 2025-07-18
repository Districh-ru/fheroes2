/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2021 - 2025                                             *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#include "ui_language.h"

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <functional>
#include <map>
#include <optional>
#include <set>
#include <utility>

#include "agg.h"
#include "agg_image.h"
#include "icn.h"
#include "settings.h"
#include "tools.h"
#include "translations.h"
#include "ui_font.h"

namespace
{
    const std::map<uint32_t, fheroes2::SupportedLanguage> languageCRC32 = { { 0x406967B9, fheroes2::SupportedLanguage::French }, // GoG version
                                                                            { 0x04745D1D, fheroes2::SupportedLanguage::German }, // GoG version
                                                                            { 0x88774771, fheroes2::SupportedLanguage::Polish }, // GoG version
                                                                            { 0xDB10FFD8, fheroes2::SupportedLanguage::Russian }, // XXI Vek version
                                                                            { 0xD5CF8AF3, fheroes2::SupportedLanguage::Russian }, // Buka version
                                                                            { 0x219B3124, fheroes2::SupportedLanguage::Italian }, // ???
                                                                            { 0x1CEBD099, fheroes2::SupportedLanguage::Czech } }; // CD Projekt

    // Strings in this map must in lower case and non translatable.
    const std::map<std::string, fheroes2::SupportedLanguage, std::less<>> languageName
        = { { "pl", fheroes2::SupportedLanguage::Polish },     { "polish", fheroes2::SupportedLanguage::Polish },
            { "de", fheroes2::SupportedLanguage::German },     { "german", fheroes2::SupportedLanguage::German },
            { "fr", fheroes2::SupportedLanguage::French },     { "french", fheroes2::SupportedLanguage::French },
            { "ru", fheroes2::SupportedLanguage::Russian },    { "russian", fheroes2::SupportedLanguage::Russian },
            { "it", fheroes2::SupportedLanguage::Italian },    { "italian", fheroes2::SupportedLanguage::Italian },
            { "cs", fheroes2::SupportedLanguage::Czech },      { "czech", fheroes2::SupportedLanguage::Czech },
            { "nb", fheroes2::SupportedLanguage::Norwegian },  { "norwegian", fheroes2::SupportedLanguage::Norwegian },
            { "be", fheroes2::SupportedLanguage::Belarusian }, { "belarusian", fheroes2::SupportedLanguage::Belarusian },
            { "uk", fheroes2::SupportedLanguage::Ukrainian },  { "ukrainian", fheroes2::SupportedLanguage::Ukrainian },
            { "bg", fheroes2::SupportedLanguage::Bulgarian },  { "bulgarian", fheroes2::SupportedLanguage::Bulgarian },
            { "es", fheroes2::SupportedLanguage::Spanish },    { "spanish", fheroes2::SupportedLanguage::Spanish },
            { "pt", fheroes2::SupportedLanguage::Portuguese }, { "portuguese", fheroes2::SupportedLanguage::Portuguese },
            { "sv", fheroes2::SupportedLanguage::Swedish },    { "swedish", fheroes2::SupportedLanguage::Swedish },
            { "tr", fheroes2::SupportedLanguage::Turkish },    { "turkish", fheroes2::SupportedLanguage::Turkish },
            { "ro", fheroes2::SupportedLanguage::Romanian },   { "romanian", fheroes2::SupportedLanguage::Romanian },
            { "nl", fheroes2::SupportedLanguage::Dutch },      { "dutch", fheroes2::SupportedLanguage::Dutch },
            { "hu", fheroes2::SupportedLanguage::Hungarian },  { "hungarian", fheroes2::SupportedLanguage::Hungarian },
            { "dk", fheroes2::SupportedLanguage::Danish },     { "danish", fheroes2::SupportedLanguage::Danish },
            { "sk", fheroes2::SupportedLanguage::Slovak },     { "slovak", fheroes2::SupportedLanguage::Slovak },
            { "vi", fheroes2::SupportedLanguage::Vietnamese }, { "vietnamese", fheroes2::SupportedLanguage::Vietnamese },
            { "gr", fheroes2::SupportedLanguage::Greek },      { "greek", fheroes2::SupportedLanguage::Greek } };
}

namespace fheroes2
{
    LanguageSwitcher::LanguageSwitcher( const SupportedLanguage language )
        : _currentLanguage( Settings::Get().getGameLanguage() )
    {
        Settings::Get().setGameLanguage( getLanguageAbbreviation( language ) );
    }

    LanguageSwitcher::~LanguageSwitcher()
    {
        Settings::Get().setGameLanguage( _currentLanguage );
    }

    SupportedLanguage getResourceLanguage()
    {
        static std::optional<SupportedLanguage> language;
        if ( language.has_value() ) {
            return *language;
        }

        const std::vector<uint8_t> & data = ::AGG::getDataFromAggFile( ICN::getIcnFileName( ICN::FONT ), false );
        if ( data.empty() ) {
            // How is it possible to run the game without a font?
            assert( 0 );
            language = SupportedLanguage::English;
            return *language;
        }

        const uint32_t crc32 = calculateCRC32( data.data(), data.size() );
        auto iter = languageCRC32.find( crc32 );
        if ( iter == languageCRC32.end() ) {
            language = SupportedLanguage::English;
            return *language;
        }

        language = iter->second;
        return *language;
    }

    std::vector<SupportedLanguage> getSupportedLanguages()
    {
        // We need to group languages by code pages to avoid recreating font related resources while switching languages.
        std::map<CodePage, std::vector<SupportedLanguage>> supportedLanguges;

        const SupportedLanguage resourceLanguage = getResourceLanguage();
        if ( resourceLanguage != SupportedLanguage::English ) {
            supportedLanguges[getCodePage( resourceLanguage )].emplace_back( resourceLanguage );
        }

        const std::set<SupportedLanguage> possibleLanguages{ SupportedLanguage::French,     SupportedLanguage::Polish,     SupportedLanguage::German,
                                                             SupportedLanguage::Russian,    SupportedLanguage::Italian,    SupportedLanguage::Norwegian,
                                                             SupportedLanguage::Belarusian, SupportedLanguage::Bulgarian,  SupportedLanguage::Ukrainian,
                                                             SupportedLanguage::Romanian,   SupportedLanguage::Spanish,    SupportedLanguage::Portuguese,
                                                             SupportedLanguage::Swedish,    SupportedLanguage::Turkish,    SupportedLanguage::Dutch,
                                                             SupportedLanguage::Hungarian,  SupportedLanguage::Czech,      SupportedLanguage::Danish,
                                                             SupportedLanguage::Slovak,     SupportedLanguage::Vietnamese, SupportedLanguage::Greek };

        for ( const SupportedLanguage language : possibleLanguages ) {
            if ( language != resourceLanguage && isAlphabetSupported( language ) ) {
                supportedLanguges[getCodePage( language )].emplace_back( language );
            }
        }

        Settings & conf = Settings::Get();

        fheroes2::SupportedLanguage currentLanguage = fheroes2::getLanguageFromAbbreviation( conf.getGameLanguage() );

        std::vector<fheroes2::SupportedLanguage> validSupportedLanguages{ fheroes2::SupportedLanguage::English };

        for ( const auto & [codePage, languages] : supportedLanguges ) {
            for ( const auto language : languages ) {
                // TODO: we shouldn't load all language resources just for the sake of verifying whether their translations exist.
                //       Find another way to avoid this heavy operation.
                if ( conf.setGameLanguage( fheroes2::getLanguageAbbreviation( language ) ) ) {
                    validSupportedLanguages.emplace_back( language );
                }
            }
        }

        conf.setGameLanguage( fheroes2::getLanguageAbbreviation( currentLanguage ) );

        assert( !validSupportedLanguages.empty() );

        return validSupportedLanguages;
    }

    const char * getLanguageName( const SupportedLanguage language )
    {
        switch ( language ) {
        case SupportedLanguage::English:
            return _( "English" );
        case SupportedLanguage::French:
            return _( "French" );
        case SupportedLanguage::Polish:
            return _( "Polish" );
        case SupportedLanguage::German:
            return _( "German" );
        case SupportedLanguage::Russian:
            return _( "Russian" );
        case SupportedLanguage::Italian:
            return _( "Italian" );
        case SupportedLanguage::Czech:
            return _( "Czech" );
        case SupportedLanguage::Norwegian:
            return _( "Norwegian" );
        case SupportedLanguage::Belarusian:
            return _( "Belarusian" );
        case SupportedLanguage::Bulgarian:
            return _( "Bulgarian" );
        case SupportedLanguage::Ukrainian:
            return _( "Ukrainian" );
        case SupportedLanguage::Romanian:
            return _( "Romanian" );
        case SupportedLanguage::Spanish:
            return _( "Spanish" );
        case SupportedLanguage::Swedish:
            return _( "Swedish" );
        case SupportedLanguage::Portuguese:
            return _( "Portuguese" );
        case SupportedLanguage::Turkish:
            return _( "Turkish" );
        case SupportedLanguage::Dutch:
            return _( "Dutch" );
        case SupportedLanguage::Hungarian:
            return _( "Hungarian" );
        case SupportedLanguage::Danish:
            return _( "Danish" );
        case SupportedLanguage::Slovak:
            return _( "Slovak" );
        case SupportedLanguage::Vietnamese:
            return _( "Vietnamese" );
        case SupportedLanguage::Greek:
            return _( "Greek" );
        default:
            // Did you add a new language? Please add the code to handle it.
            assert( 0 );
            return nullptr;
        }
    }

    const char * getLanguageAbbreviation( const SupportedLanguage language )
    {
        switch ( language ) {
        case SupportedLanguage::English:
            return ""; // English is a special case. It always returns an empty string as it's a default language.
        case SupportedLanguage::French:
            return "fr";
        case SupportedLanguage::Polish:
            return "pl";
        case SupportedLanguage::German:
            return "de";
        case SupportedLanguage::Russian:
            return "ru";
        case SupportedLanguage::Italian:
            return "it";
        case SupportedLanguage::Czech:
            return "cs";
        case SupportedLanguage::Norwegian:
            return "nb";
        case SupportedLanguage::Belarusian:
            return "be";
        case SupportedLanguage::Bulgarian:
            return "bg";
        case SupportedLanguage::Ukrainian:
            return "uk";
        case SupportedLanguage::Romanian:
            return "ro";
        case SupportedLanguage::Spanish:
            return "es";
        case SupportedLanguage::Swedish:
            return "sv";
        case SupportedLanguage::Portuguese:
            return "pt";
        case SupportedLanguage::Turkish:
            return "tr";
        case SupportedLanguage::Dutch:
            return "nl";
        case SupportedLanguage::Hungarian:
            return "hu";
        case SupportedLanguage::Danish:
            return "dk";
        case SupportedLanguage::Slovak:
            return "sk";
        case SupportedLanguage::Vietnamese:
            return "vi";
        case SupportedLanguage::Greek:
            return "gr";
        default:
            // Did you add a new language? Please add the code to handle it.
            assert( 0 );
            return nullptr;
        }
    }

    SupportedLanguage getLanguageFromAbbreviation( const std::string & abbreviation )
    {
        if ( abbreviation.empty() ) {
            return SupportedLanguage::English;
        }

        const std::string name( StringLower( abbreviation ) );

        auto iter = languageName.find( name );
        if ( iter == languageName.end() ) {
            // Unsupported language. Fallback to English.
            return SupportedLanguage::English;
        }

        return iter->second;
    }

    void updateAlphabet( const std::string & abbreviation )
    {
        const SupportedLanguage language = getLanguageFromAbbreviation( abbreviation );
        const SupportedLanguage resourceLanguage = getResourceLanguage();

        // The original French assets replaces several ASCII special characters with language-specific characters.
        // In the engine we use CP1252 for these characters.
        if ( ( language == SupportedLanguage::English ) && ( resourceLanguage == SupportedLanguage::French ) ) {
            // Force generate CP1252 alphabet when English language is selected for French assets.
            AGG::updateLanguageDependentResources( SupportedLanguage::French, false );
        }
        else {
            // To generate CP1252 alphabet for French assets we must assume that these assets are not original.
            // Also we force generation of Cyrillic symbols for Russian assets - it is done for consistency of fonts with different assets.
            const bool isOriginalResourceLanguage
                = ( language == SupportedLanguage::English )
                  || ( language == resourceLanguage && resourceLanguage != SupportedLanguage::French && resourceLanguage != SupportedLanguage::Russian );

            AGG::updateLanguageDependentResources( language, isOriginalResourceLanguage );
        }
    }

    SupportedLanguage getCurrentLanguage()
    {
        return fheroes2::getLanguageFromAbbreviation( Settings::Get().getGameLanguage() );
    }

    CodePage getCodePage( const SupportedLanguage language )
    {
        switch ( language ) {
        case SupportedLanguage::English:
            return CodePage::ASCII;
        case SupportedLanguage::Czech:
        case SupportedLanguage::Hungarian:
        case SupportedLanguage::Polish:
        case SupportedLanguage::Slovak:
            return CodePage::CP1250;
        case SupportedLanguage::Belarusian:
        case SupportedLanguage::Bulgarian:
        case SupportedLanguage::Russian:
        case SupportedLanguage::Ukrainian:
            return CodePage::CP1251;
        case SupportedLanguage::Danish:
        case SupportedLanguage::Dutch:
        case SupportedLanguage::French:
        case SupportedLanguage::German:
        case SupportedLanguage::Italian:
        case SupportedLanguage::Norwegian:
        case SupportedLanguage::Portuguese:
        case SupportedLanguage::Spanish:
        case SupportedLanguage::Swedish:
            return CodePage::CP1252;
        case SupportedLanguage::Greek:
            return CodePage::CP1253;
        case SupportedLanguage::Turkish:
            return CodePage::CP1254;
        case SupportedLanguage::Vietnamese:
            return CodePage::CP1258;
        case SupportedLanguage::Romanian:
            return CodePage::ISO8859_16;
        default:
            // Add new language handling code!
            assert( 0 );
            break;
        }

        return CodePage::ASCII;
    }
}
