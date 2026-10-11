#include "global.h"
#include "bg.h"
#include "data.h"
#include "event_object_movement.h"
#include "field_player_avatar.h"
#include "game_options.h"
#include "link.h"
#include "pokemon.h"
#include "pokedex.h"
#include "random.h"
#include "trainer.h"
#include "menu.h"
#include "move.h"
#include "overworld.h"
#include "pokemon_storage_system.h"
#include "sound.h"
#include "strings.h"
#include "text.h"
#include "window.h"
#include "wild_encounter.h"

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

static const struct WindowTemplate sGameOptionsWindowTemplate =
{
    .bg = 0,
    .tilemapLeft = 1,
    .tilemapTop = 1,
    .width = 28,
    .height = 19,
    .paletteNum = 15,
    .baseBlock = 0x200,
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
static const u8 sText_AllGenerations[] = _("All");
static const u8 sText_Generation[] = _("Gen {STR_VAR_1}");
static const u8 sText_RandomScaled[] = _("Scaled");
static const u8 sText_RandomTyped[] = _("Typed");
static const u8 sText_ExpOne[] = _("1x");
static const u8 sText_ExpTwo[] = _("2x");
static const u8 sText_ExpHundred[] = _("100x");

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
    sText_GameOptionsPc,
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
    GAME_OPTION_PC_ANYWHERE,
    GAME_OPTION_EXP_SHARE,
};

static const u8 *const sOnOffValues[] = {sText_Off, sText_On};
static u8 sGameOptionsWindowId;
static TaskFunc sGameOptionsNextFunc;
static bool8 sGameOptionsInField;
static bool8 sOpenPcOnClose;
static bool8 sNuzlockeEncounterPending;
static bool8 sNuzlockeAreaWasUsed;
static bool8 sNuzlockeEncounterCaught;
static u8 sNuzlockeFaintedPartySlots;
static bool8 sGameplaySpeciesCandidatesInitialized;
static bool8 sGameplaySpeciesCandidates[NUM_SPECIES];

static void InitGameplaySpeciesCandidates(void)
{
    static bool8 seenNationalDex[NATIONAL_DEX_COUNT + 1];

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
            if (dexNum > NATIONAL_DEX_COUNT || seenNationalDex[dexNum])
                continue;
            seenNationalDex[dexNum] = TRUE;
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

static enum Species ChooseGameplaySpecies(enum Species originalSpecies, enum Type requiredType, bool8 waterOnly, bool8 scaled, u16 range)
{
    enum Species chosen = SPECIES_NONE;
    u32 candidates = 0;
    u32 targetBst = GetSpeciesBaseStatTotal(originalSpecies);
    u8 generation = GetGameOption(GAME_OPTION_GENERATION);

    for (enum Species species = SPECIES_BULBASAUR; species < NUM_SPECIES; species++)
    {
        u32 bst;
        if (!IsGameplaySpeciesAllowed(species, generation, requiredType, waterOnly))
            continue;
        bst = GetSpeciesBaseStatTotal(species);
        if (scaled && (bst + range < targetBst || bst > targetBst + range))
            continue;
        candidates++;
        if (Random() % candidates == 0)
            chosen = species;
    }
    return chosen;
}

enum Species GetGameplaySpecies(enum Species originalSpecies, enum Type requiredType, bool8 waterOnly)
{
    u8 randomMode = GetGameOption(GAME_OPTION_RANDOM_POKEMON);
    u8 generation = GetGameOption(GAME_OPTION_GENERATION);
    bool8 originalAllowed;
    bool8 scaled;
    enum Species species;

    if (originalSpecies < SPECIES_BULBASAUR || originalSpecies >= NUM_SPECIES)
        return originalSpecies;
    originalAllowed = IsGameplaySpeciesAllowed(originalSpecies, generation, requiredType, waterOnly);
    if (randomMode == 0 && originalAllowed)
        return originalSpecies;

    scaled = randomMode == 2 || (randomMode == 0 && (generation != 0 || (waterOnly && !originalAllowed)));
    species = ChooseGameplaySpecies(originalSpecies, requiredType, waterOnly, scaled, 50);
    if (species == SPECIES_NONE && scaled)
        species = ChooseGameplaySpecies(originalSpecies, requiredType, waterOnly, TRUE, 100);
    if (species == SPECIES_NONE)
        species = ChooseGameplaySpecies(originalSpecies, requiredType, waterOnly, FALSE, 0);
    if (species == SPECIES_NONE && requiredType != TYPE_NONE)
        species = ChooseGameplaySpecies(originalSpecies, TYPE_NONE, waterOnly, FALSE, 0);
    return species == SPECIES_NONE ? originalSpecies : species;
}

enum Species GetGameplayTrainerSpecies(enum Species originalSpecies, enum Type requiredType)
{
    u8 generation = GetGameOption(GAME_OPTION_GENERATION);

    if (generation != 0 || GetGameOption(GAME_OPTION_RANDOM_POKEMON) != 0)
        return GetGameplaySpecies(originalSpecies, requiredType, FALSE);

    // Trainers get cross-generation species by default. Keep their original type theme
    // and look for a similar base-stat total so the battle pacing stays familiar.
    for (u16 range = 50; range <= 200; range *= 2)
    {
        enum Species chosen = SPECIES_NONE;
        u32 candidates = 0;
        u32 targetBst = GetSpeciesBaseStatTotal(originalSpecies);

        for (enum Species species = SPECIES_BULBASAUR; species < NUM_SPECIES; species++)
        {
            if (GetSpeciesGeneration(species) == GetSpeciesGeneration(originalSpecies)
             || !IsGameplaySpeciesAllowed(species, 0, requiredType, FALSE))
                continue;
            u32 bst = GetSpeciesBaseStatTotal(species);
            if (bst + range < targetBst || bst > targetBst + range)
                continue;
            candidates++;
            if (Random() % candidates == 0)
                chosen = species;
        }
        if (chosen != SPECIES_NONE)
            return chosen;
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

    if (!GetGameOption(GAME_OPTION_NUZLOCKE) || sNuzlockeEncounterPending)
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
    if (!GetGameOption(GAME_OPTION_NUZLOCKE) || isShiny)
        return FALSE;
    return sNuzlockeEncounterCaught
        || sNuzlockeAreaWasUsed
        || GetSetPokedexFlag(SpeciesToNationalPokedexNum(species), FLAG_GET_CAUGHT);
}

bool8 IsNuzlockeEncounterCatchable(enum Species species, bool8 isShiny)
{
    return GetGameOption(GAME_OPTION_NUZLOCKE)
        && sNuzlockeEncounterPending
        && !IsNuzlockeCatchBlocked(species, isShiny);
}

void MarkNuzlockeEncounterCaught(void)
{
    if (GetGameOption(GAME_OPTION_NUZLOCKE) && sNuzlockeEncounterPending)
        sNuzlockeEncounterCaught = TRUE;
}

void MarkNuzlockePartyMonFainted(u8 partyIndex)
{
    if (GetGameOption(GAME_OPTION_NUZLOCKE) && partyIndex < PARTY_SIZE)
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

    for (enum Move move = MOVE_NONE + 1; move < NUM_MOVES; move++)
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
            return sText_AllGenerations;
        ConvertIntToDecimalStringN(gStringVar1, value, STR_CONV_MODE_LEFT_ALIGN, 1);
        StringExpandPlaceholders(gStringVar4, sText_Generation);
        return gStringVar4;
    case GAME_OPTION_RANDOM_POKEMON:
        return value == 0 ? sText_Off : (value == 1 ? sText_On : sText_RandomScaled);
    case GAME_OPTION_RANDOM_MOVES:
        return value == 0 ? sText_Off : (value == 1 ? sText_On : sText_RandomTyped);
    case GAME_OPTION_EXP_MULTIPLIER:
        return value == 0 ? sText_ExpOne : (value == 1 ? sText_ExpTwo : sText_ExpHundred);
    default:
        return value ? sText_On : sText_Off;
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
        AddTextPrinterParameterized(sGameOptionsWindowId, FONT_NORMAL, sText_NuzlockeRulesHint, 20, 142, TEXT_SKIP_DRAW, NULL);
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
        u8 y = 34 + i * 13;
        if (i == cursor)
            AddTextPrinterParameterized(sGameOptionsWindowId, FONT_NORMAL, gText_SelectorArrow, 5, y, TEXT_SKIP_DRAW, NULL);
        AddTextPrinterParameterized(sGameOptionsWindowId, FONT_NORMAL, labels[i], 20, y, TEXT_SKIP_DRAW, NULL);
        AddTextPrinterParameterized(sGameOptionsWindowId, FONT_NORMAL, GetOptionValueText(optionIds[i]), 196, y, TEXT_SKIP_DRAW, NULL);
    }

    AddTextPrinterParameterized(sGameOptionsWindowId, FONT_NORMAL, sText_GameOptionsHint, 22, 142, TEXT_SKIP_DRAW, NULL);
    CopyWindowToVram(sGameOptionsWindowId, COPYWIN_FULL);
}

static void CloseGameOptionsMenu(u8 taskId)
{
    ClearWindowTilemap(sGameOptionsWindowId);
    CopyWindowToVram(sGameOptionsWindowId, COPYWIN_MAP);
    RemoveWindow(sGameOptionsWindowId);
    CopyBgTilemapBufferToVram(0);
    if (sGameOptionsInField)
    {
        UnlockPlayerFieldControls();
        PlayerUnfreeze();
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
        sGameOptionsWindowId = AddWindow(&sGameOptionsWindowTemplate);
        PutWindowTilemap(sGameOptionsWindowId);
        gTasks[taskId].data[0] = 1;
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
        u8 rows = gTasks[taskId].data[1] == 0 ? ARRAY_COUNT(sGameOptionsPage1Ids) : ARRAY_COUNT(sGameOptionsPage2Ids);
        gTasks[taskId].data[2] = (gTasks[taskId].data[2] + rows - 1) % rows;
        PlaySE(SE_SELECT);
        PrintGameOptions(taskId);
    }
    else if (JOY_NEW(DPAD_DOWN))
    {
        u8 rows = gTasks[taskId].data[1] == 0 ? ARRAY_COUNT(sGameOptionsPage1Ids) : ARRAY_COUNT(sGameOptionsPage2Ids);
        gTasks[taskId].data[2] = (gTasks[taskId].data[2] + 1) % rows;
        PlaySE(SE_SELECT);
        PrintGameOptions(taskId);
    }
    else if (JOY_NEW(A_BUTTON | DPAD_LEFT))
    {
        if (JOY_NEW(A_BUTTON) && gTasks[taskId].data[1] == 1 && gTasks[taskId].data[2] == 1 && GetGameOption(GAME_OPTION_PC_ANYWHERE))
        {
            sOpenPcOnClose = TRUE;
            CloseGameOptionsMenu(taskId);
            return;
        }
        ChangeSelectedOption(taskId, -1);
        PlaySE(SE_SELECT);
        PrintGameOptions(taskId);
    }
    else if (JOY_NEW(DPAD_RIGHT))
    {
        ChangeSelectedOption(taskId, 1);
        PlaySE(SE_SELECT);
        PrintGameOptions(taskId);
    }
    else if (JOY_NEW(SELECT_BUTTON))
    {
        gTasks[taskId].data[3] = TRUE;
        PlaySE(SE_SELECT);
        PrintGameOptions(taskId);
    }
    else if (JOY_NEW(L_BUTTON | R_BUTTON))
    {
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
