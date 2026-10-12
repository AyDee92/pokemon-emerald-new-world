#ifndef GUARD_GAME_OPTIONS_H
#define GUARD_GAME_OPTIONS_H

#include "task.h"
#include "pokemon.h"

struct Trainer;

enum GameOptionId
{
    GAME_OPTION_LEVEL_CAP,
    GAME_OPTION_NUZLOCKE,
    GAME_OPTION_GENERATION,
    GAME_OPTION_RANDOM_POKEMON,
    GAME_OPTION_RANDOM_MOVES,
    GAME_OPTION_RANDOM_ABILITIES,
    GAME_OPTION_HEAL_AFTER_BATTLE,
    GAME_OPTION_EXP_MULTIPLIER,
    GAME_OPTION_INFINITE_REPEL,
    GAME_OPTION_PC_ANYWHERE,
    GAME_OPTION_EXP_SHARE,
};

u8 GetGameOption(enum GameOptionId option);
void SetGameOption(enum GameOptionId option, u8 value);
enum Species GetGameplaySpecies(enum Species originalSpecies, enum Type requiredType, bool8 waterOnly);
enum Species GetGameplayTrainerSpecies(enum Species originalSpecies, enum Type requiredType);
enum Type GetGameplayTrainerTheme(const struct Trainer *trainer);
void ApplyGameplayMonOptions(struct Pokemon *mon, enum Species species);
enum Move GetGameplayRandomMove(enum Species species, const enum Move *excludedMoves, u8 excludedCount);
enum Ability GetGameplayAbility(u32 personality, enum Species species, u8 abilityNum);
void BeginNuzlockeWildEncounter(void);
bool8 IsNuzlockeCatchBlocked(enum Species species, bool8 isShiny);
bool8 IsNuzlockeEncounterCatchable(enum Species species, bool8 isShiny);
bool8 IsNuzlockeChallengeActive(void);
void ActivateNuzlockeChallenge(void);
void MarkNuzlockeEncounterCaught(void);
void MarkNuzlockePartyMonFainted(u8 partyIndex);
bool8 IsNuzlockePartyMonFainted(u8 partyIndex);
void ResetNuzlockeFaintedPartySlots(void);
bool8 RetireNuzlockePartyMon(struct Pokemon *mon);
bool8 IsNuzlockePokemonRetired(struct Pokemon *mon);
void FinishNuzlockeBattle(void);
void StartNewGameOptionsMenu(u8 taskId, TaskFunc nextTask);
void OpenFieldOptionsMenu(void);

#endif // GUARD_GAME_OPTIONS_H
