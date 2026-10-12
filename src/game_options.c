#include "global.h"
#include "bg.h"
#include "data.h"
#include "event_object_movement.h"
#include "field_player_avatar.h"
#include "game_options.h"
#include "link.h"
#include "main.h"
#include "pokemon.h"
#include "pokedex.h"
#include "palette.h"
#include "random.h"
#include "script.h"
#include "string_util.h"
#include "trainer.h"
#include "menu.h"
#include "move.h"
#include "overworld.h"
#include "pokemon_storage_system.h"
#include "sound.h"
#include "strings.h"
#include "text.h"
#include "text_window.h"
#include "window.h"
#include "wild_encounter.h"
#include "constants/map_groups.h"
#include "constants/songs.h"
#include "data/map_group_count.h"

enum
{
    OPTION_LEVEL_CAP,
    OPTION_NUZLOCKE,
    OPTION_GENERATION,
    OPTION_RANDOM_POKEMON,
    OPTION_RANDOM_MOVES,
    OPTION_RANDOM_ABILITIES,
    OPTION_HEAL_AFTER_BATTLE,
    OPTION_EXP_MULTIPLIER,
    OPTION_INFINITE_REPEL,
    OPTION_PC_ANYWHERE,
    OPTION_EXP_SHARE,
};

#define OPTION_SHIFT_LEVEL_CAP           0
#define OPTION_SHIFT_NUZLOCKE            1
#define OPTION_SHIFT_GENERATION          2
#define OPTION_SHIFT_RANDOM_POKEMON      6
#define OPTION_SHIFT_RANDOM_MOVES        8
#define OPTION_SHIFT_RANDOM_ABILITIES    10
#define OPTION_SHIFT_HEAL_AFTER_BATTLE   11
#define OPTION_SHIFT_EXP_MULTIPLIER      12
#define OPTION_SHIFT_INFINITE_REPEL      14
#define OPTION_SHIFT_PC_ANYWHERE         15
#define OPTION_SHIFT_EXP_SHARE           16
#define GAMEPLAY_OPTIONS_SIGNATURE       0x47504F54 // "GPOT"
#define GAMEPLAY_OPTIONS_NUZLOCKE_STARTED (1u << 17)

static const struct WindowTemplate sGameOptionsWindowTemplate =
{
    .bg = 0,
    .tilemapLeft = 1,
    .tilemapTop = 1,
    .width = 28,
    .height = 19,
    .paletteNum = 15,
    // Stay within BG0's tile range. 0x200 was beyond the 10-bit tile index
    // space once the 28x19 window was added and corrupted the bottom of the UI.
    .baseBlock = 0x139,
};

static const struct WindowTemplate sFieldOptionsWindowTemplate =
{
    .bg = 0,
    .tilemapLeft = 1,
    .tilemapTop = 1,
    .width = 18,
    .height = 8,
    .paletteNum = 15,
    .baseBlock = 0x139,
};

static const u8 sText_GameOptionsTitle[] = _("GAME SETTINGS");
static const u8 sText_GameOptionsPage1[] = _("CHALLENGE AND RANDOMIZER");
static const u8 sText_GameOptionsPage2[] = _("FIELD SETTINGS");
static const u8 sText_GameOptionsHint[] = _("A: Edit  L/R: Page  SELECT: Rules");
static const u8 sText_NuzlockeRules[] = _("First new species per area.\nFleeing uses that slot. Fainted\nPokémon are boxed permanently.\nShinies may always be caught.");
static const u8 sText_NuzlockeRulesHint[] = _("A/B: Return");
static const u8 sText_GameOptionsLevelCap[] = _("Level Cap");
static const u8 sText_GameOptionsNuzlocke[] = _("Nuzlocke");
static const u8 sText_GameOptionsGeneration[] = _("Pokémon Generation");
static const u8 sText_GameOptionsPokemon[] = _("Random Pokémon");
static const u8 sText_GameOptionsMoves[] = _("Random Moves");
static const u8 sText_GameOptionsAbilities[] = _("Random Abilities");
static const u8 sText_GameOptionsHeal[] = _("Heal After Battle");
static const u8 sText_GameOptionsExp[] = _("Exp. Gain");
static const u8 sText_GameOptionsRepel[] = _("Infinite Repel");
static const u8 sText_GameOptionsPc[] = _("Access PC");
static const u8 sText_GameOptionsExpShare[] = _("Exp. Share");
static const u8 sText_Off[] = _("Off");
static const u8 sText_On[] = _("On");
static const u8 sText_OffGreen[] = _("{COLOR GREEN}{SHADOW LIGHT_GREEN}Off");
static const u8 sText_OnGreen[] = _("{COLOR GREEN}{SHADOW LIGHT_GREEN}On");
static const u8 sText_AllGenerations[] = _("All");
static const u8 sText_Generation[] = _("Gen {STR_VAR_1}");
static const u8 sText_RandomScaled[] = _("Scaled");
static const u8 sText_RandomTyped[] = _("Typed");
static const u8 sText_ExpOne[] = _("1x");
static const u8 sText_ExpTwo[] = _("2x");
static const u8 sText_ExpHundred[] = _("100x");
static const u8 sText_AllGenerationsGreen[] = _("{COLOR GREEN}{SHADOW LIGHT_GREEN}All");
static const u8 sText_GenerationGreen[] = _("{COLOR GREEN}{SHADOW LIGHT_GREEN}Gen {STR_VAR_1}");
static const u8 sText_RandomScaledGreen[] = _("{COLOR GREEN}{SHADOW LIGHT_GREEN}Scaled");
static const u8 sText_RandomTypedGreen[] = _("{COLOR GREEN}{SHADOW LIGHT_GREEN}Typed");
static const u8 sText_ExpOneGreen[] = _("{COLOR GREEN}{SHADOW LIGHT_GREEN}1x");
static const u8 sText_ExpTwoGreen[] = _("{COLOR GREEN}{SHADOW LIGHT_GREEN}2x");
static const u8 sText_ExpHundredGreen[] = _("{COLOR GREEN}{SHADOW LIGHT_GREEN}100x");

static const u8 *const sGameOptionsPage1[] =
{
    sText_GameOptionsLevelCap,
    sText_GameOptionsNuzlocke,
    sText_GameOptionsGeneration,
    sText_GameOptionsPokemon,
    sText_GameOptionsMoves,
    sText_GameOptionsAbilities,
    sText_GameOptionsHeal,
    sText_GameOptionsExp,
};

static const u8 *const sGameOptionsPage2[] =
{
    sText_GameOptionsRepel,
    sText_GameOptionsExpShare,
};

static const enum GameOptionId sGameOptionsPage1Ids[] =
{
    GAME_OPTION_LEVEL_CAP,
    GAME_OPTION_NUZLOCKE,
    GAME_OPTION_GENERATION,
    GAME_OPTION_RANDOM_POKEMON,
    GAME_OPTION_RANDOM_MOVES,
    GAME_OPTION_RANDOM_ABILITIES,
    GAME_OPTION_HEAL_AFTER_BATTLE,
    GAME_OPTION_EXP_MULTIPLIER,
};

static const enum GameOptionId sGameOptionsPage2Ids[] =
{
    GAME_OPTION_INFINITE_REPEL,
    GAME_OPTION_EXP_SHARE,
};

static const u8 *const sOnOffValues[] = {sText_Off, sText_On};
static EWRAM_DATA u8 sGameOptionsWindowId;
static EWRAM_DATA TaskFunc sGameOptionsNextFunc;
static EWRAM_DATA bool8 sGameOptionsInField;
static EWRAM_DATA bool8 sOpenPcOnClose;
static EWRAM_DATA bool8 sNuzlockeEncounterPending;
static EWRAM_DATA bool8 sNuzlockeAreaWasUsed;
static EWRAM_DATA bool8 sNuzlockeEncounterCaught;
static EWRAM_DATA u8 sNuzlockeFaintedPartySlots;
static EWRAM_DATA bool8 sGameplaySpeciesCandidatesInitialized;
static EWRAM_DATA bool8 sGameplaySpeciesCandidates[NUM_SPECIES];
static EWRAM_DATA bool8 sGameplaySpeciesSeenNationalDex[NATIONAL_DEX_COUNT + 1];

static const enum Species sGameplayStarterSpecies[][3] =
{
    {SPECIES_BULBASAUR, SPECIES_CHARMANDER, SPECIES_SQUIRTLE},
    {SPECIES_CHIKORITA, SPECIES_CYNDAQUIL, SPECIES_TOTODILE},
    {SPECIES_TREECKO, SPECIES_TORCHIC, SPECIES_MUDKIP},
    {SPECIES_TURTWIG, SPECIES_CHIMCHAR, SPECIES_PIPLUP},
    {SPECIES_SNIVY, SPECIES_TEPIG, SPECIES_OSHAWOTT},
    {SPECIES_CHESPIN, SPECIES_FENNEKIN, SPECIES_FROAKIE},
    {SPECIES_ROWLET, SPECIES_LITTEN, SPECIES_POPPLIO},
    {SPECIES_GROOKEY, SPECIES_SCORBUNNY, SPECIES_SOBBLE},
    {SPECIES_SPRIGATITO, SPECIES_FUECOCO, SPECIES_QUAXLY},
};

static void InitGameplaySpeciesCandidates(void)
{
    if (sGameplaySpeciesCandidatesInitialized)
        return;

    for (enum Species species = SPECIES_BULBASAUR; species < NUM_SPECIES; species++)
    {
        const struct SpeciesInfo *info = &gSpeciesInfo[species];
        u16 dexNum = info->natDexNum;
        bool8 isRegionalForm = info->isAlolanForm || info->isGalarianForm || info->isHisuianForm || info->isPaldeanForm;

        if (dexNum == NATIONAL_DEX_NONE || info->isMegaEvolution || info->isPrimalReversion || info->isUltraBurst
         || info->isGigantamax || info->isTeraForm || info->isTotem)
            continue;
        if (!isRegionalForm)
        {
            if (dexNum > NATIONAL_DEX_COUNT || sGameplaySpeciesSeenNationalDex[dexNum])
                continue;
            sGameplaySpeciesSeenNationalDex[dexNum] = TRUE;
        }
        sGameplaySpeciesCandidates[species] = TRUE;
    }

    sGameplaySpeciesCandidatesInitialized = TRUE;
}

static u8 GetOptionShift(enum GameOptionId option)
{
    static const u8 shifts[] =
    {
        OPTION_SHIFT_LEVEL_CAP,
        OPTION_SHIFT_NUZLOCKE,
        OPTION_SHIFT_GENERATION,
        OPTION_SHIFT_RANDOM_POKEMON,
        OPTION_SHIFT_RANDOM_MOVES,
        OPTION_SHIFT_RANDOM_ABILITIES,
        OPTION_SHIFT_HEAL_AFTER_BATTLE,
        OPTION_SHIFT_EXP_MULTIPLIER,
        OPTION_SHIFT_INFINITE_REPEL,
        OPTION_SHIFT_PC_ANYWHERE,
        OPTION_SHIFT_EXP_SHARE,
    };

    return shifts[option];
}

static u32 GetOptionMask(enum GameOptionId option)
{
    switch (option)
    {
    case GAME_OPTION_GENERATION:
        return 15;
    case GAME_OPTION_RANDOM_POKEMON:
    case GAME_OPTION_RANDOM_MOVES:
    case GAME_OPTION_EXP_MULTIPLIER:
        return 3;
    default:
        return 1;
    }
}

u8 GetGameOption(enum GameOptionId option)
{
    if (gSaveBlock2Ptr->gameplayOptions.signature != GAMEPLAY_OPTIONS_SIGNATURE)
        return 0;
    return (gSaveBlock2Ptr->gameplayOptions.settings >> GetOptionShift(option)) & GetOptionMask(option);
}

void SetGameOption(enum GameOptionId option, u8 value)
{
    u8 shift = GetOptionShift(option);
    u32 mask = GetOptionMask(option);

    if (gSaveBlock2Ptr->gameplayOptions.signature != GAMEPLAY_OPTIONS_SIGNATURE)
        gSaveBlock2Ptr->gameplayOptions.settings = 0;
    gSaveBlock2Ptr->gameplayOptions.signature = GAMEPLAY_OPTIONS_SIGNATURE;
    gSaveBlock2Ptr->gameplayOptions.settings = (gSaveBlock2Ptr->gameplayOptions.settings & ~(mask << shift)) | ((value & mask) << shift);
}

bool8 IsNuzlockeChallengeActive(void)
{
    return GetGameOption(GAME_OPTION_NUZLOCKE)
        && (gSaveBlock2Ptr->gameplayOptions.settings & GAMEPLAY_OPTIONS_NUZLOCKE_STARTED);
}

void ActivateNuzlockeChallenge(void)
{
    if (!GetGameOption(GAME_OPTION_NUZLOCKE) || IsNuzlockeChallengeActive())
        return;

    gSaveBlock2Ptr->gameplayOptions.settings |= GAMEPLAY_OPTIONS_NUZLOCKE_STARTED;
    sNuzlockeEncounterPending = FALSE;
    sNuzlockeAreaWasUsed = FALSE;
    sNuzlockeEncounterCaught = FALSE;
    sNuzlockeFaintedPartySlots = 0;
}

static u8 GetOptionValueCount(enum GameOptionId option)
{
    switch (option)
    {
    case GAME_OPTION_GENERATION:
        return 10;
    case GAME_OPTION_RANDOM_POKEMON:
    case GAME_OPTION_RANDOM_MOVES:
    case GAME_OPTION_EXP_MULTIPLIER:
        return 3;
    default:
        return 2;
    }
}

static u8 GetSpeciesGeneration(enum Species species)
{
    u16 dexNum = gSpeciesInfo[species].natDexNum;

    if (gSpeciesInfo[species].isAlolanForm)
        return 7;
    if (gSpeciesInfo[species].isGalarianForm || gSpeciesInfo[species].isHisuianForm)
        return 8;
    if (gSpeciesInfo[species].isPaldeanForm)
        return 9;
    if (dexNum == NATIONAL_DEX_NONE)
        return 0;
    if (dexNum <= NATIONAL_DEX_MEW)
        return 1;
    if (dexNum <= NATIONAL_DEX_CELEBI)
        return 2;
    if (dexNum <= NATIONAL_DEX_DEOXYS)
        return 3;
    if (dexNum <= NATIONAL_DEX_ARCEUS)
        return 4;
    if (dexNum <= NATIONAL_DEX_GENESECT)
        return 5;
    if (dexNum <= NATIONAL_DEX_VOLCANION)
        return 6;
    if (dexNum <= NATIONAL_DEX_MELMETAL)
        return 7;
    if (dexNum <= NATIONAL_DEX_ENAMORUS)
        return 8;
    return 9;
}

static bool8 IsGameplaySpeciesAllowed(enum Species species, u8 generation, enum Type requiredType, bool8 waterOnly)
{
    InitGameplaySpeciesCandidates();
    if (species < SPECIES_BULBASAUR || species >= NUM_SPECIES || gSpeciesInfo[species].natDexNum == NATIONAL_DEX_NONE)
        return FALSE;
    if (!sGameplaySpeciesCandidates[species])
        return FALSE;
    if (gSpeciesInfo[species].isMegaEvolution || gSpeciesInfo[species].isPrimalReversion || gSpeciesInfo[species].isUltraBurst
        || gSpeciesInfo[species].isGigantamax || gSpeciesInfo[species].isTeraForm || gSpeciesInfo[species].isTotem)
        return FALSE;
    if (generation != 0 && GetSpeciesGeneration(species) != generation)
        return FALSE;
    if (waterOnly && GetSpeciesType(species, 0) != TYPE_WATER && GetSpeciesType(species, 1) != TYPE_WATER)
        return FALSE;
    if (requiredType != TYPE_NONE && GetSpeciesType(species, 0) != requiredType && GetSpeciesType(species, 1) != requiredType)
        return FALSE;
    return TRUE;
}

static bool8 IsGameplayLegendary(enum Species species)
{
    const struct SpeciesInfo *info = &gSpeciesInfo[species];
    return info->isRestrictedLegendary || info->isSubLegendary || info->isMythical;
}

static bool8 IsGameplayStarter(enum Species species)
{
    enum Species baseSpecies = species;

    for (u32 stage = 0; stage < 3; stage++)
    {
        for (u32 generation = 0; generation < ARRAY_COUNT(sGameplayStarterSpecies); generation++)
        {
            for (u32 starter = 0; starter < ARRAY_COUNT(sGameplayStarterSpecies[generation]); starter++)
            {
                if (baseSpecies == sGameplayStarterSpecies[generation][starter])
                    return TRUE;
            }
        }
        baseSpecies = GetSpeciesPreEvolution(baseSpecies);
        if (baseSpecies == SPECIES_NONE)
            break;
    }

    return FALSE;
}

static u32 GetGameplaySpeciesHash(enum Species originalSpecies, u8 generation, enum Type requiredType, bool8 waterOnly)
{
    // A stable hash makes non-randomizer substitutions consistent across encounters and battles.
    return (originalSpecies * 1103515245u + generation * 12345u + requiredType * 2654435761u + waterOnly * 97u) >> 8;
}

static enum Species ChooseGameplaySpecies(enum Species originalSpecies, enum Type requiredType, bool8 waterOnly, bool8 scaled, u16 range, bool8 keepLegendary, bool8 starterOnly, bool8 randomize)
{
    enum Species chosen = SPECIES_NONE;
    u32 candidates = 0;
    u32 targetBst = GetSpeciesBaseStatTotal(originalSpecies);
    u8 generation = GetGameOption(GAME_OPTION_GENERATION);
    u32 selection;

    for (enum Species species = SPECIES_BULBASAUR; species < NUM_SPECIES; species++)
    {
        u32 bst;
        if (!IsGameplaySpeciesAllowed(species, generation, requiredType, waterOnly))
            continue;
        if (keepLegendary != IsGameplayLegendary(species))
            continue;
        if (starterOnly && !IsGameplayStarter(species))
            continue;
        bst = GetSpeciesBaseStatTotal(species);
        if (scaled && (bst + range < targetBst || bst > targetBst + range))
            continue;
        candidates++;
        if (randomize && Random() % candidates == 0)
            chosen = species;
    }

    if (!randomize && candidates != 0)
    {
        selection = GetGameplaySpeciesHash(originalSpecies, generation, requiredType, waterOnly) % candidates;
        for (enum Species species = SPECIES_BULBASAUR; species < NUM_SPECIES; species++)
        {
            u32 bst;
            if (!IsGameplaySpeciesAllowed(species, generation, requiredType, waterOnly)
             || keepLegendary != IsGameplayLegendary(species)
             || (starterOnly && !IsGameplayStarter(species)))
                continue;
            bst = GetSpeciesBaseStatTotal(species);
            if (scaled && (bst + range < targetBst || bst > targetBst + range))
                continue;
            if (selection-- == 0)
                return species;
        }
    }
    return chosen;
}

enum Species GetGameplaySpecies(enum Species originalSpecies, enum Type requiredType, bool8 waterOnly)
{
    u8 randomMode = GetGameOption(GAME_OPTION_RANDOM_POKEMON);
    u8 generation = GetGameOption(GAME_OPTION_GENERATION);
    bool8 originalAllowed;
    bool8 scaled;
    bool8 randomize = randomMode != 0;
    bool8 keepLegendary;
    enum Species species;

    if (originalSpecies < SPECIES_BULBASAUR || originalSpecies >= NUM_SPECIES)
        return originalSpecies;
    originalAllowed = IsGameplaySpeciesAllowed(originalSpecies, generation, requiredType, waterOnly);
    if (randomMode == 0 && originalAllowed)
        return originalSpecies;
    if (requiredType == TYPE_NONE)
        requiredType = GetSpeciesType(originalSpecies, 0);
    keepLegendary = IsGameplayLegendary(originalSpecies);

    scaled = randomMode == 2 || (randomMode == 0 && (generation != 0 || (waterOnly && !originalAllowed)));
    species = ChooseGameplaySpecies(originalSpecies, requiredType, waterOnly, scaled, 50, keepLegendary, FALSE, randomize);
    if (species == SPECIES_NONE && scaled)
        species = ChooseGameplaySpecies(originalSpecies, requiredType, waterOnly, TRUE, 100, keepLegendary, FALSE, randomize);
    if (species == SPECIES_NONE)
        species = ChooseGameplaySpecies(originalSpecies, requiredType, waterOnly, FALSE, 0, keepLegendary, FALSE, randomize);
    if (species == SPECIES_NONE && requiredType != TYPE_NONE)
        species = ChooseGameplaySpecies(originalSpecies, TYPE_NONE, waterOnly, FALSE, 0, keepLegendary, FALSE, randomize);
    return species == SPECIES_NONE ? originalSpecies : species;
}

enum Species GetGameplayTrainerSpecies(enum Species originalSpecies, enum Type requiredType)
{
    u8 generation = GetGameOption(GAME_OPTION_GENERATION);
    bool8 randomize = GetGameOption(GAME_OPTION_RANDOM_POKEMON) != 0;
    bool8 keepLegendary = IsGameplayLegendary(originalSpecies);

    if (IsGameplayStarter(originalSpecies))
    {
        enum Type starterType = GetSpeciesType(originalSpecies, 0);
        if (generation != 0 && !randomize
         && IsGameplaySpeciesAllowed(originalSpecies, generation, starterType, FALSE))
            return originalSpecies;

        enum Species starter = ChooseGameplaySpecies(originalSpecies, starterType, FALSE, FALSE, 0,
                                                      keepLegendary, TRUE, randomize);
        if (starter != SPECIES_NONE)
            return starter;
    }

    if (generation != 0 || GetGameOption(GAME_OPTION_RANDOM_POKEMON) != 0)
        return GetGameplaySpecies(originalSpecies, requiredType, FALSE);

    // Trainers get cross-generation species by default. Keep their original type theme
    // and look for a similar base-stat total so the battle pacing stays familiar.
    for (u16 range = 50; range <= 200; range *= 2)
    {
        u32 candidates = 0;
        u32 targetBst = GetSpeciesBaseStatTotal(originalSpecies);
        u32 selection;

        for (enum Species species = SPECIES_BULBASAUR; species < NUM_SPECIES; species++)
        {
            if (GetSpeciesGeneration(species) == GetSpeciesGeneration(originalSpecies)
             || !IsGameplaySpeciesAllowed(species, 0, requiredType, FALSE))
                continue;
            if (keepLegendary != IsGameplayLegendary(species))
                continue;
            u32 bst = GetSpeciesBaseStatTotal(species);
            if (bst + range < targetBst || bst > targetBst + range)
                continue;
            candidates++;
        }
        if (candidates == 0)
            continue;

        selection = GetGameplaySpeciesHash(originalSpecies, 0, requiredType, FALSE) % candidates;
        for (enum Species species = SPECIES_BULBASAUR; species < NUM_SPECIES; species++)
        {
            if (GetSpeciesGeneration(species) == GetSpeciesGeneration(originalSpecies)
             || !IsGameplaySpeciesAllowed(species, 0, requiredType, FALSE)
             || keepLegendary != IsGameplayLegendary(species))
                continue;
            u32 bst = GetSpeciesBaseStatTotal(species);
            if (bst + range < targetBst || bst > targetBst + range)
                continue;
            if (selection-- == 0)
                return species;
        }
    }

    return GetGameplaySpecies(originalSpecies, requiredType, FALSE);
}

enum Type GetGameplayTrainerTheme(const struct Trainer *trainer)
{
    u8 typeCounts[TYPE_STELLAR + 1] = {0};
    enum Type mostCommonType = TYPE_NONE;
    u8 maxCount = 0;
    u16 partyCount = trainer->poolSize ? trainer->poolSize : trainer->partySize;

    for (u16 i = 0; i < partyCount; i++)
    {
        enum Type type1 = GetSpeciesType(trainer->party[i].species, 0);
        enum Type type2 = GetSpeciesType(trainer->party[i].species, 1);
        if (type1 != TYPE_NONE && type1 <= TYPE_STELLAR)
            typeCounts[type1]++;
        if (type2 != TYPE_NONE && type2 <= TYPE_STELLAR && type2 != type1)
            typeCounts[type2]++;
    }
    for (u8 type = 1; type <= TYPE_STELLAR; type++)
    {
        if (typeCounts[type] > maxCount)
        {
            maxCount = typeCounts[type];
            mostCommonType = type;
        }
    }
    return maxCount >= 2 ? mostCommonType : TYPE_NONE;
}

void BeginNuzlockeWildEncounter(void)
{
    u32 areaId;
    u16 mapGroup = gSaveBlock1Ptr->location.mapGroup;
    u16 mapNum = gSaveBlock1Ptr->location.mapNum;
    bool8 hasNewSpecies = FALSE;
    u8 mask;

    if (!IsNuzlockeChallengeActive() || sNuzlockeEncounterPending)
        return;
    if (mapGroup >= ARRAY_COUNT(MAP_GROUP_COUNT) || mapNum >= MAP_GROUP_COUNT[mapGroup])
        return;

    areaId = mapNum;
    for (u16 group = 0; group < mapGroup; group++)
        areaId += MAP_GROUP_COUNT[group];
    if (areaId >= sizeof(gSaveBlock1Ptr->nuzlockeEncounterAreas) * 8)
        return;

    for (u8 i = 0; i < PARTY_SIZE; i++)
    {
        enum Species species = GetMonData(&gParties[B_TRAINER_OPPONENT_A][i], MON_DATA_SPECIES);
        if (species != SPECIES_NONE && species != SPECIES_EGG
         && !GetSetPokedexFlag(SpeciesToNationalPokedexNum(species), FLAG_GET_CAUGHT))
            hasNewSpecies = TRUE;
    }

    sNuzlockeEncounterPending = TRUE;
    sNuzlockeEncounterCaught = FALSE;
    mask = 1 << (areaId % 8);
    sNuzlockeAreaWasUsed = (gSaveBlock1Ptr->nuzlockeEncounterAreas[areaId / 8] & mask) != 0;

    // Any new species in a simultaneous encounter claims the slot, including
    // when the player flees. Duplicate-only encounters leave the area open.
    if (!sNuzlockeAreaWasUsed && hasNewSpecies)
        gSaveBlock1Ptr->nuzlockeEncounterAreas[areaId / 8] |= mask;
}

bool8 IsNuzlockeCatchBlocked(enum Species species, bool8 isShiny)
{
    if (!IsNuzlockeChallengeActive() || isShiny)
        return FALSE;
    return sNuzlockeEncounterCaught
        || sNuzlockeAreaWasUsed
        || GetSetPokedexFlag(SpeciesToNationalPokedexNum(species), FLAG_GET_CAUGHT);
}

bool8 IsNuzlockeEncounterCatchable(enum Species species, bool8 isShiny)
{
    return IsNuzlockeChallengeActive()
        && sNuzlockeEncounterPending
        && !IsNuzlockeCatchBlocked(species, isShiny);
}

void MarkNuzlockeEncounterCaught(void)
{
    if (IsNuzlockeChallengeActive() && sNuzlockeEncounterPending)
        sNuzlockeEncounterCaught = TRUE;
}

void MarkNuzlockePartyMonFainted(u8 partyIndex)
{
    if (IsNuzlockeChallengeActive() && partyIndex < PARTY_SIZE)
        sNuzlockeFaintedPartySlots |= 1 << partyIndex;
}

bool8 IsNuzlockePartyMonFainted(u8 partyIndex)
{
    return partyIndex < PARTY_SIZE && (sNuzlockeFaintedPartySlots & (1 << partyIndex));
}

bool8 IsNuzlockePokemonRetired(struct Pokemon *mon)
{
    return mon->box.nuzlockeRetired && mon->box.nuzlockeRetiredSignature;
}

bool8 RetireNuzlockePartyMon(struct Pokemon *mon)
{
    mon->box.nuzlockeRetired = TRUE;
    mon->box.nuzlockeRetiredSignature = TRUE;

    for (u8 boxId = 0; boxId < TOTAL_BOXES_COUNT; boxId++)
    {
        s16 boxPosition = GetFirstFreeBoxSpot(boxId);
        if (boxPosition >= 0)
        {
            SetBoxMonAt(boxId, boxPosition, &mon->box);
            return TRUE;
        }
    }
    return FALSE;
}

void ResetNuzlockeFaintedPartySlots(void)
{
    sNuzlockeFaintedPartySlots = 0;
}

void FinishNuzlockeBattle(void)
{
    sNuzlockeEncounterPending = FALSE;
    sNuzlockeAreaWasUsed = FALSE;
    sNuzlockeEncounterCaught = FALSE;
    sNuzlockeFaintedPartySlots = 0;
}

static bool8 IsMoveAlreadyChosen(const enum Move *moves, enum Move move, u8 count)
{
    for (u8 i = 0; i < count; i++)
    {
        if (moves[i] == move)
            return TRUE;
    }
    return FALSE;
}

static enum Move ChooseRandomMove(enum Species species, bool8 preferType, const enum Move *chosenMoves, u8 chosenCount)
{
    enum Move chosen = MOVE_NONE;
    u32 candidateCount = 0;
    enum Type primaryType = GetSpeciesType(species, 0);
    enum Type secondaryType = GetSpeciesType(species, 1);

    for (enum Move move = MOVE_NONE + 1; move < MOVES_COUNT_ALL; move++)
    {
        if (move == MOVE_STRUGGLE || IsMoveAlreadyChosen(chosenMoves, move, chosenCount))
            continue;
        if (preferType && GetMoveType(move) != primaryType && GetMoveType(move) != secondaryType)
            continue;
        candidateCount++;
        if (Random() % candidateCount == 0)
            chosen = move;
    }

    return chosen;
}

enum Move GetGameplayRandomMove(enum Species species, const enum Move *excludedMoves, u8 excludedCount)
{
    bool8 preferType = GetGameOption(GAME_OPTION_RANDOM_MOVES) == 2 && Random() % 4 != 0;
    enum Move move = ChooseRandomMove(species, preferType, excludedMoves, excludedCount);

    if (move == MOVE_NONE && preferType)
        move = ChooseRandomMove(species, FALSE, excludedMoves, excludedCount);
    return move;
}

enum Ability GetGameplayAbility(u32 personality, enum Species species, u8 abilityNum)
{
    if (GetGameOption(GAME_OPTION_RANDOM_ABILITIES))
        return 1 + personality % (ABILITIES_COUNT - 1);
    return GetAbilityBySpecies(species, abilityNum);
}

void ApplyGameplayMonOptions(struct Pokemon *mon, enum Species species)
{
    u8 randomMoveMode = GetGameOption(GAME_OPTION_RANDOM_MOVES);
    enum Move moves[MAX_MON_MOVES] = {MOVE_NONE};

    if (randomMoveMode == 0)
        return;

    for (u8 i = 0; i < MAX_MON_MOVES; i++)
    {
        moves[i] = GetGameplayRandomMove(species, moves, i);
        if (moves[i] == MOVE_NONE)
            break;
        SetMonData(mon, MON_DATA_MOVE1 + i, &moves[i]);
        u32 pp = GetMovePP(moves[i]);
        SetMonData(mon, MON_DATA_PP1 + i, &pp);
    }
    for (u8 i = 0; i < MAX_MON_MOVES; i++)
    {
        if (moves[i] == MOVE_NONE)
        {
            u16 noMove = MOVE_NONE;
            u8 noPp = 0;
            SetMonData(mon, MON_DATA_MOVE1 + i, &noMove);
            SetMonData(mon, MON_DATA_PP1 + i, &noPp);
        }
    }
}

static void ChangeSelectedOption(u8 taskId, s8 delta)
{
    const enum GameOptionId *optionIds;
    u8 count;
    u8 row = gTasks[taskId].data[2];
    u8 option;
    s16 value;
    
    if (gTasks[taskId].data[1] == 0)
    {
        optionIds = sGameOptionsPage1Ids;
        count = ARRAY_COUNT(sGameOptionsPage1Ids);
    }
    else
    {
        optionIds = sGameOptionsPage2Ids;
        count = ARRAY_COUNT(sGameOptionsPage2Ids);
    }

    if (row >= count)
        row = 0;
    option = optionIds[row];
    value = GetGameOption(option) + delta;
    if (value < 0)
        value = GetOptionValueCount(option) - 1;
    else if (value >= GetOptionValueCount(option))
        value = 0;
    SetGameOption(option, value);
}

static const u8 *GetOptionValueText(enum GameOptionId option)
{
    u8 value = GetGameOption(option);

    switch (option)
    {
    case GAME_OPTION_GENERATION:
        if (value == 0)
            return sGameOptionsInField ? sText_AllGenerations : sText_AllGenerationsGreen;
        ConvertIntToDecimalStringN(gStringVar1, value, STR_CONV_MODE_LEFT_ALIGN, 1);
        StringExpandPlaceholders(gStringVar4, sGameOptionsInField ? sText_Generation : sText_GenerationGreen);
        return gStringVar4;
    case GAME_OPTION_RANDOM_POKEMON:
        return value == 0 ? (sGameOptionsInField ? sText_Off : sText_OffGreen)
            : (value == 1 ? (sGameOptionsInField ? sText_On : sText_OnGreen) : (sGameOptionsInField ? sText_RandomScaled : sText_RandomScaledGreen));
    case GAME_OPTION_RANDOM_MOVES:
        return value == 0 ? (sGameOptionsInField ? sText_Off : sText_OffGreen)
            : (value == 1 ? (sGameOptionsInField ? sText_On : sText_OnGreen) : (sGameOptionsInField ? sText_RandomTyped : sText_RandomTypedGreen));
    case GAME_OPTION_EXP_MULTIPLIER:
        return value == 0 ? (sGameOptionsInField ? sText_ExpOne : sText_ExpOneGreen)
            : (value == 1 ? (sGameOptionsInField ? sText_ExpTwo : sText_ExpTwoGreen) : (sGameOptionsInField ? sText_ExpHundred : sText_ExpHundredGreen));
    default:
        return sGameOptionsInField ? (value ? sText_OnGreen : sText_Off) : (value ? sText_OnGreen : sText_OffGreen);
    }
}

static void PrintGameOptions(u8 taskId)
{
    const u8 *const *labels;
    const enum GameOptionId *optionIds;
    u8 count;
    u8 page = gTasks[taskId].data[1];
    u8 cursor = gTasks[taskId].data[2];
    u8 i;
    
    FillWindowPixelBuffer(sGameOptionsWindowId, PIXEL_FILL(1));
    DrawStdWindowFrame(sGameOptionsWindowId, FALSE);
    AddTextPrinterParameterized(sGameOptionsWindowId, FONT_NORMAL, sText_GameOptionsTitle, 8, 4, TEXT_SKIP_DRAW, NULL);
    if (gTasks[taskId].data[3])
    {
        AddTextPrinterParameterized(sGameOptionsWindowId, FONT_NORMAL, sText_NuzlockeRules, 16, 42, TEXT_SKIP_DRAW, NULL);
        AddTextPrinterParameterized(sGameOptionsWindowId, FONT_NORMAL, sText_NuzlockeRulesHint, 20, 110, TEXT_SKIP_DRAW, NULL);
        CopyWindowToVram(sGameOptionsWindowId, COPYWIN_FULL);
        return;
    }
    AddTextPrinterParameterized(sGameOptionsWindowId, FONT_NORMAL, page == 0 ? sText_GameOptionsPage1 : sText_GameOptionsPage2, 8, 18, TEXT_SKIP_DRAW, NULL);

    if (page == 0)
    {
        labels = sGameOptionsPage1;
        optionIds = sGameOptionsPage1Ids;
        count = ARRAY_COUNT(sGameOptionsPage1);
    }
    else
    {
        labels = sGameOptionsPage2;
        optionIds = sGameOptionsPage2Ids;
        count = ARRAY_COUNT(sGameOptionsPage2);
    }

    for (i = 0; i < count; i++)
    {
        u8 y = 30 + i * 12;
        if (i == cursor)
            AddTextPrinterParameterized(sGameOptionsWindowId, FONT_NORMAL, gText_SelectorArrow, 5, y, TEXT_SKIP_DRAW, NULL);
        AddTextPrinterParameterized(sGameOptionsWindowId, FONT_NORMAL, labels[i], 20, y, TEXT_SKIP_DRAW, NULL);
        AddTextPrinterParameterized(sGameOptionsWindowId, FONT_NORMAL, GetOptionValueText(optionIds[i]), 196, y, TEXT_SKIP_DRAW, NULL);
    }

    AddTextPrinterParameterized(sGameOptionsWindowId, FONT_NORMAL, sText_GameOptionsHint, 22, 128, TEXT_SKIP_DRAW, NULL);
    CopyWindowToVram(sGameOptionsWindowId, COPYWIN_FULL);
}

static void PrintFieldOptions(u8 taskId)
{
    static const u8 *const labels[] =
    {
        sText_GameOptionsRepel,
        sText_GameOptionsPc,
        sText_GameOptionsExpShare,
    };
    u8 cursor = gTasks[taskId].data[2];

    FillWindowPixelBuffer(sGameOptionsWindowId, PIXEL_FILL(1));
    DrawStdWindowFrame(sGameOptionsWindowId, FALSE);
    for (u8 i = 0; i < ARRAY_COUNT(labels); i++)
    {
        u8 y = 9 + i * 16;
        if (i == cursor)
            AddTextPrinterParameterized(sGameOptionsWindowId, FONT_NORMAL, gText_SelectorArrow, 0, y, TEXT_SKIP_DRAW, NULL);
        AddTextPrinterParameterized(sGameOptionsWindowId, FONT_NORMAL, labels[i], 8, y, TEXT_SKIP_DRAW, NULL);
        if (i != 1)
        {
            enum GameOptionId option = i == 0 ? GAME_OPTION_INFINITE_REPEL : GAME_OPTION_EXP_SHARE;
            AddTextPrinterParameterized(sGameOptionsWindowId, FONT_NORMAL, GetOptionValueText(option), 116, y, TEXT_SKIP_DRAW, NULL);
        }
    }
    CopyWindowToVram(sGameOptionsWindowId, COPYWIN_FULL);
}

static void CloseGameOptionsMenu(u8 taskId)
{
    // Clear both the standard frame and the window contents before freeing its
    // tile allocation, otherwise the border remains visible on the field.
    ClearStdWindowAndFrame(sGameOptionsWindowId, TRUE);
    RemoveWindow(sGameOptionsWindowId);
    CopyBgTilemapBufferToVram(0);
    if (sGameOptionsInField)
    {
        UnlockPlayerFieldControls();
        UnfreezeObjectEvents();
        DestroyTask(taskId);
        if (sOpenPcOnClose)
        {
            sOpenPcOnClose = FALSE;
            ShowPokemonStorageSystemPC();
        }
    }
    else
    {
        gTasks[taskId].func = sGameOptionsNextFunc;
        gTasks[taskId].data[0] = 0;
        gTasks[taskId].data[1] = 0;
        gTasks[taskId].data[2] = 0;
        gTasks[taskId].data[3] = 0;
    }
}

static void Task_GameOptionsMenu(u8 taskId)
{
    if (gTasks[taskId].data[0] == 0)
    {
        LoadMessageBoxGfx(0, 0xF3, BG_PLTT_ID(15));
        sGameOptionsWindowId = AddWindow(sGameOptionsInField ? &sFieldOptionsWindowTemplate : &sGameOptionsWindowTemplate);
        PutWindowTilemap(sGameOptionsWindowId);
        gTasks[taskId].data[0] = 1;
        if (sGameOptionsInField)
            PrintFieldOptions(taskId);
        else
            PrintGameOptions(taskId);
        return;
    }

    if (gTasks[taskId].data[3])
    {
        if (JOY_NEW(A_BUTTON | B_BUTTON | SELECT_BUTTON))
        {
            gTasks[taskId].data[3] = 0;
            PlaySE(SE_SELECT);
            PrintGameOptions(taskId);
        }
        return;
    }

    if (JOY_NEW(DPAD_UP))
    {
        u8 rows = sGameOptionsInField ? 3 : (gTasks[taskId].data[1] == 0 ? ARRAY_COUNT(sGameOptionsPage1Ids) : ARRAY_COUNT(sGameOptionsPage2Ids));
        gTasks[taskId].data[2] = (gTasks[taskId].data[2] + rows - 1) % rows;
        PlaySE(SE_SELECT);
        if (sGameOptionsInField)
            PrintFieldOptions(taskId);
        else
            PrintGameOptions(taskId);
    }
    else if (JOY_NEW(DPAD_DOWN))
    {
        u8 rows = sGameOptionsInField ? 3 : (gTasks[taskId].data[1] == 0 ? ARRAY_COUNT(sGameOptionsPage1Ids) : ARRAY_COUNT(sGameOptionsPage2Ids));
        gTasks[taskId].data[2] = (gTasks[taskId].data[2] + 1) % rows;
        PlaySE(SE_SELECT);
        if (sGameOptionsInField)
            PrintFieldOptions(taskId);
        else
            PrintGameOptions(taskId);
    }
    else if (JOY_NEW(A_BUTTON | DPAD_LEFT))
    {
        if (sGameOptionsInField && JOY_NEW(A_BUTTON) && gTasks[taskId].data[2] == 1)
        {
            sOpenPcOnClose = TRUE;
            CloseGameOptionsMenu(taskId);
            return;
        }
        if (sGameOptionsInField)
        {
            if (gTasks[taskId].data[2] == 0)
                SetGameOption(GAME_OPTION_INFINITE_REPEL, !GetGameOption(GAME_OPTION_INFINITE_REPEL));
            else if (gTasks[taskId].data[2] == 2)
                SetGameOption(GAME_OPTION_EXP_SHARE, !GetGameOption(GAME_OPTION_EXP_SHARE));
        }
        else
            ChangeSelectedOption(taskId, JOY_NEW(A_BUTTON) ? 1 : -1);
        PlaySE(SE_SELECT);
        if (sGameOptionsInField)
            PrintFieldOptions(taskId);
        else
            PrintGameOptions(taskId);
    }
    else if (JOY_NEW(DPAD_RIGHT))
    {
        if (sGameOptionsInField)
        {
            if (gTasks[taskId].data[2] == 0)
                SetGameOption(GAME_OPTION_INFINITE_REPEL, !GetGameOption(GAME_OPTION_INFINITE_REPEL));
            else if (gTasks[taskId].data[2] == 2)
                SetGameOption(GAME_OPTION_EXP_SHARE, !GetGameOption(GAME_OPTION_EXP_SHARE));
        }
        else
            ChangeSelectedOption(taskId, 1);
        PlaySE(SE_SELECT);
        if (sGameOptionsInField)
            PrintFieldOptions(taskId);
        else
            PrintGameOptions(taskId);
    }
    else if (JOY_NEW(SELECT_BUTTON) && !sGameOptionsInField)
    {
        gTasks[taskId].data[3] = TRUE;
        PlaySE(SE_SELECT);
        PrintGameOptions(taskId);
    }
    else if (JOY_NEW(L_BUTTON | R_BUTTON))
    {
        if (sGameOptionsInField)
            return;
        gTasks[taskId].data[1] ^= 1;
        gTasks[taskId].data[2] = 0;
        PlaySE(SE_SELECT);
        PrintGameOptions(taskId);
    }
    else if (JOY_NEW(START_BUTTON))
    {
        PlaySE(SE_SELECT);
        CloseGameOptionsMenu(taskId);
    }
    else if (JOY_NEW(B_BUTTON))
    {
        PlaySE(SE_SELECT);
        CloseGameOptionsMenu(taskId);
    }
}

void StartNewGameOptionsMenu(u8 taskId, TaskFunc nextTask)
{
    gSaveBlock2Ptr->gameplayOptions.settings = 0;
    gSaveBlock2Ptr->gameplayOptions.signature = GAMEPLAY_OPTIONS_SIGNATURE;
    sNuzlockeEncounterPending = FALSE;
    sNuzlockeAreaWasUsed = FALSE;
    sNuzlockeEncounterCaught = FALSE;
    sNuzlockeFaintedPartySlots = 0;
    sGameOptionsInField = FALSE;
    sOpenPcOnClose = FALSE;
    sGameOptionsNextFunc = nextTask;
    gTasks[taskId].func = Task_GameOptionsMenu;
    gTasks[taskId].data[0] = 0;
    gTasks[taskId].data[1] = 0;
    gTasks[taskId].data[2] = 0;
    gTasks[taskId].data[3] = 0;
}

void OpenFieldOptionsMenu(void)
{
    u8 taskId;

    if (InUnionRoom() || IsOverworldLinkActive())
        return;
    sOpenPcOnClose = FALSE;
    FreezeObjectEvents();
    PlayerFreeze();
    StopPlayerAvatar();
    LockPlayerFieldControls();
    taskId = CreateTask(Task_GameOptionsMenu, 0);
    sGameOptionsInField = TRUE;
    gTasks[taskId].data[0] = 0;
    gTasks[taskId].data[1] = 1;
    gTasks[taskId].data[2] = 0;
    gTasks[taskId].data[3] = 0;
}
