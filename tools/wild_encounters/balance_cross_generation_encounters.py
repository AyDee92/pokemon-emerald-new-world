#!/usr/bin/env python3
"""Distribute every enabled National Dex species through the encounter tables.

Encounter levels and rates are preserved. Species are assigned by ascending
base-stat total to slots with the nearest original species BST, which keeps
each encounter area's existing power curve while covering the full dex.
Water and fishing slots are populated exclusively with Water-type species.
"""

import argparse
import json
import re
import subprocess
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
SPECIES_DIR = ROOT / "src/data/pokemon/species_info"
ENCOUNTERS_PATH = ROOT / "src/data/wild_encounters.json"
GEN_FILES = [SPECIES_DIR / f"gen_{generation}_families.h" for generation in range(1, 10)]
PREPROCESSOR_VALUES = {
    "P_UPDATED_STATS": 8,  # GEN_LATEST in include/config/pokemon.h
    "P_REGIONAL_FORMS": 1,
    "P_FUSION_FORMS": 1,
    "P_MEGA_EVOLUTIONS": 1,
    "P_PRIMAL_REVERSIONS": 1,
    "P_ULTRA_BURST_FORMS": 1,
    "P_GIGANTAMAX_FORMS": 1,
    "P_TERA_FORMS": 1,
    "TRUE": 1,
    "FALSE": 0,
}
PREPROCESSOR_VALUES.update({f"GEN_{generation}": generation - 1 for generation in range(1, 10)})
PREPROCESSOR_VALUES.update({f"P_GEN_{generation}_POKEMON": 1 for generation in range(1, 10)})


def evaluate_condition(expression):
    expression = re.sub(r"!(?!=)", " not ", expression)
    expression = expression.replace("&&", " and ").replace("||", " or ")
    expression = re.sub(
        r"\b[A-Z][A-Z0-9_]*\b",
        lambda match: str(PREPROCESSOR_VALUES.get(match.group(0), 1)),
        expression,
    )
    try:
        return bool(eval(expression, {"__builtins__": None}, {}))
    except Exception:
        return True


def select_active_preprocessor_lines(source):
    output = []
    stack = []
    active = True
    for line in source.splitlines():
        directive = re.match(r"\s*#\s*(if|ifdef|ifndef|elif|else|endif)\b(.*)", line)
        if directive is None:
            if active:
                output.append(line)
            continue

        command, expression = directive.groups()
        if command in ("if", "ifdef", "ifndef"):
            condition = evaluate_condition(expression) if command == "if" else True
            if command == "ifndef":
                condition = not condition
            stack.append({"parent": active, "taken": condition, "active": active and condition})
            active = stack[-1]["active"]
        elif command == "elif":
            frame = stack[-1]
            condition = not frame["taken"] and evaluate_condition(expression)
            frame["active"] = frame["parent"] and condition
            frame["taken"] = frame["taken"] or condition
            active = frame["active"]
        elif command == "else":
            frame = stack[-1]
            frame["active"] = frame["parent"] and not frame["taken"]
            frame["taken"] = True
            active = frame["active"]
        else:
            frame = stack.pop()
            active = frame["parent"]
    return "\n".join(output)


def read_active_macros(source):
    active_source = select_active_preprocessor_lines(source).replace("\\\n", " ")
    return {
        match.group(1): match.group(2).strip()
        for match in re.finditer(r"^\s*#define\s+([A-Z][A-Z0-9_]*)\s+([^\n]+)", active_source, re.MULTILINE)
    }


def expand_macros(expression, macros):
    for _ in range(20):
        expanded = re.sub(
            r"\b[A-Z][A-Z0-9_]*\b",
            lambda match: str(PREPROCESSOR_VALUES[match.group(0)])
            if match.group(0) in PREPROCESSOR_VALUES
            else f"({macros[match.group(0)]})" if match.group(0) in macros
            else match.group(0),
            expression,
        )
        if expanded == expression:
            break
        expression = expanded
    return expression


def _find_top_level_ternary(expression):
    depth = 0
    question = -1
    nested = 0

    for index, char in enumerate(expression):
        if char == "(":
            depth += 1
        elif char == ")":
            depth -= 1
        elif depth == 0 and char == "?":
            if question < 0:
                question = index
            else:
                nested += 1
        elif depth == 0 and char == ":" and question >= 0:
            if nested == 0:
                return question, index
            nested -= 1

    return None


def evaluate_integer_expression(expression, macros):
    expression = re.sub(r"/\*.*?\*/|//.*", "", expression).strip()
    expression = expression.replace("\\\n", " ")

    expression = expand_macros(expression, macros)

    while expression.startswith("(") and expression.endswith(")"):
        depth = 0
        wraps_entire_expression = True
        for index, char in enumerate(expression):
            if char == "(":
                depth += 1
            elif char == ")":
                depth -= 1
                if depth == 0 and index != len(expression) - 1:
                    wraps_entire_expression = False
                    break
        if not wraps_entire_expression:
            break
        expression = expression[1:-1].strip()

    ternary = _find_top_level_ternary(expression)
    if ternary is not None:
        question, colon = ternary
        condition = expression[:question].strip()
        true_value = expression[question + 1:colon].strip()
        false_value = expression[colon + 1:].strip()
        condition = condition.replace("&&", " and ").replace("||", " or ")
        condition = re.sub(r"!(?!=)", " not ", condition)
        if eval(condition, {"__builtins__": None}, {}):
            return evaluate_integer_expression(true_value, macros)
        return evaluate_integer_expression(false_value, macros)

    expression = expression.replace("&&", " and ").replace("||", " or ")
    expression = re.sub(r"!(?!=)", " not ", expression)
    value = eval(expression, {"__builtins__": None}, {})
    if not isinstance(value, (int, bool)):
        raise ValueError(f"Stat expression is not an integer: {expression}")
    return int(value)


def read_species_catalog():
    catalog = {}
    entry_pattern = re.compile(r"\[(SPECIES_[A-Z0-9_]+)\]\s*=\s*\{")

    for source_path in GEN_FILES:
        source = source_path.read_text(encoding="utf-8")
        active_macros = read_active_macros(source)
        for match in entry_pattern.finditer(source):
            start = match.end() - 1
            depth = 0
            end = start
            for end in range(start, len(source)):
                if source[end] == "{":
                    depth += 1
                elif source[end] == "}":
                    depth -= 1
                    if depth == 0:
                        break
            block = select_active_preprocessor_lines(source[start:end + 1])
            dex_match = re.search(r"\.natDexNum\s*=\s*NATIONAL_DEX_([A-Z0-9_]+)", block)
            stats = [
                evaluate_integer_expression(value, active_macros)
                for value in re.findall(r"\.base(?:HP|Attack|Defense|Speed|SpAttack|SpDefense)\s*=\s*([^,\n]+)", block)
            ]
            types_match = re.search(r"\.types\s*=\s*([^,\n]+(?:\([^)]*\))?)", block)
            if dex_match is None or not stats or types_match is None:
                continue
            excluded = dex_match.group(1) == "NONE" or any(re.search(rf"\.{flag}\s*=\s*TRUE", block) for flag in (
                "isMegaEvolution", "isPrimalReversion", "isUltraBurst", "isGigantamax", "isTeraForm", "isTotem"
            ))
            type_expr = types_match.group(1)
            has_water = "TYPE_WATER" in expand_macros(type_expr, active_macros)
            regional = any(re.search(rf"\.{flag}\s*=\s*TRUE", block) for flag in (
                "isAlolanForm", "isGalarianForm", "isHisuianForm", "isPaldeanForm"
            ))
            catalog[match.group(1)] = {
                "bst": sum(stats),
                "water": has_water,
                "dex": dex_match.group(1),
                "regional": regional,
                "eligible": not excluded,
            }

    canonical_catalog = {}
    seen_dex = set()
    for species, info in catalog.items():
        if not info["eligible"]:
            continue
        if info["regional"]:
            canonical_catalog[species] = info
        elif info["dex"] not in seen_dex:
            canonical_catalog[species] = info
            seen_dex.add(info["dex"])
        else:
            info["eligible"] = False

    if not canonical_catalog:
        raise RuntimeError("No species were read from the species data files")
    for species, info in canonical_catalog.items():
        catalog[species]["eligible"] = True
    return catalog


def get_bst(species, catalog):
    if species.startswith("SPECIES_UNOWN"):
        return 336
    try:
        return catalog[species]["bst"]
    except KeyError as error:
        raise RuntimeError(f"Encounter species {species} has no parsed base stats") from error


def match_species_to_closest_slots(ordered_species, slots, catalog):
    """Place every species into a distinct slot with minimum total BST drift.

    On a one-dimensional stat scale, an optimal assignment preserves BST order.
    Dynamic programming chooses which slots to use, reserving the closest slots
    for the species that need them instead of letting early greedy choices use
    up those slots.
    """
    species_count = len(ordered_species)
    slot_count = len(slots)
    width = slot_count + 1
    infinity = 0x7FFFFFFF
    choices = bytearray((species_count + 1) * width)
    previous = [0] * width

    for species_index, species in enumerate(ordered_species, start=1):
        current = [infinity] * width
        species_bst = catalog[species]["bst"]
        for slot_index in range(1, width):
            skip_slot_cost = current[slot_index - 1]
            match_slot_cost = previous[slot_index - 1] + abs(species_bst - slots[slot_index - 1][1])
            if match_slot_cost < skip_slot_cost:
                current[slot_index] = match_slot_cost
                choices[species_index * width + slot_index] = 1
            else:
                current[slot_index] = skip_slot_cost
        previous = current

    matches = []
    species_index = species_count
    slot_index = slot_count
    while species_index:
        if slot_index == 0:
            raise RuntimeError("Encounter slots could not cover every species")
        if choices[species_index * width + slot_index]:
            matches.append((species_index - 1, slot_index - 1))
            species_index -= 1
            slot_index -= 1
        else:
            slot_index -= 1
    return matches


def rebalance_slots(encounters, field_name, candidates, catalog, baseline_encounters=None):
    slots = []
    for encounter_index, encounter in enumerate(encounters):
        field = encounter.get(field_name)
        if field is None:
            continue
        baseline_encounter = None
        if baseline_encounters and encounter_index < len(baseline_encounters):
            candidate = baseline_encounters[encounter_index]
            if candidate.get("map") == encounter.get("map"):
                baseline_encounter = candidate
        baseline_field = (baseline_encounter or {}).get(field_name)
        for index, mon in enumerate(field["mons"]):
            if baseline_field and baseline_field["mons"]:
                baseline_mon = baseline_field["mons"][index % len(baseline_field["mons"])]
                target_bst = get_bst(baseline_mon["species"], catalog)
            else:
                target_bst = get_bst(mon["species"], catalog)
            slots.append((mon, target_bst, encounter))

    if len(slots) < len(candidates):
        raise RuntimeError(f"{field_name} has {len(slots)} slots for {len(candidates)} required species")

    slots.sort(key=lambda item: item[1])
    ordered_species = sorted(candidates, key=lambda species: catalog[species]["bst"])
    used_by_encounter = {id(encounter): set() for encounter in encounters}

    # Match all eligible species to distinct slots with minimum total BST drift.
    assigned_slots = set()
    for species_index, slot_index in match_species_to_closest_slots(ordered_species, slots, catalog):
        species = ordered_species[species_index]
        mon, _, encounter = slots[slot_index]
        mon["species"] = species
        used_by_encounter[id(encounter)].add(species)
        assigned_slots.add(slot_index)

    # Fill each encounter's remaining slots with the closest unused species,
    # keeping its table as diverse as the available species pool allows.
    for slot_index, (mon, slot_bst, encounter) in enumerate(slots):
        if slot_index in assigned_slots:
            continue
        used = used_by_encounter[id(encounter)]
        available = [species for species in ordered_species if species not in used]
        if not available:
            raise RuntimeError(f"{field_name} encounter has more slots than distinct species")
        selected = min(available, key=lambda species: abs(catalog[species]["bst"] - slot_bst))
        mon["species"] = selected
        used.add(selected)

    return len(slots)


def validate_encounters(encounters, field_name, candidates, catalog, require_distinct=False):
    candidate_set = set(candidates)
    covered_species = set()

    for encounter in encounters:
        field = encounter.get(field_name)
        if field is None:
            continue
        row_species = [mon["species"] for mon in field["mons"]]
        invalid_species = set(row_species) - candidate_set
        if invalid_species:
            raise RuntimeError(f"{field_name} contains ineligible species: {sorted(invalid_species)}")
        if require_distinct and len(row_species) != len(set(row_species)):
            raise RuntimeError(f"{field_name} contains duplicate species in {encounter.get('map')}")
        covered_species.update(row_species)

    missing_species = candidate_set - covered_species
    if missing_species:
        raise RuntimeError(f"{field_name} does not cover every eligible species: {sorted(missing_species)}")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--baseline-ref",
        help="read encounter slot species from a Git ref while writing the current JSON file",
    )
    args = parser.parse_args()
    catalog = read_species_catalog()
    payload = json.loads(ENCOUNTERS_PATH.read_text(encoding="utf-8"))
    baseline_encounters = None
    if args.baseline_ref:
        relative_path = ENCOUNTERS_PATH.relative_to(ROOT).as_posix()
        baseline = subprocess.check_output(
            ["git", "show", f"{args.baseline_ref}:{relative_path}"],
            cwd=ROOT,
            text=True,
        )
        baseline_encounters = json.loads(baseline)["wild_encounter_groups"][0]["encounters"]
    encounters = payload["wild_encounter_groups"][0]["encounters"]
    all_species = sorted(species for species, info in catalog.items() if info["eligible"])
    water_species = [species for species in all_species if catalog[species]["water"]]

    slot_counts = {
        "land_mons": rebalance_slots(encounters, "land_mons", all_species, catalog, baseline_encounters),
        "water_mons": rebalance_slots(encounters, "water_mons", water_species, catalog, baseline_encounters),
        "fishing_mons": rebalance_slots(encounters, "fishing_mons", water_species, catalog, baseline_encounters),
    }

    validate_encounters(encounters, "land_mons", all_species, catalog, require_distinct=True)
    validate_encounters(encounters, "water_mons", water_species, catalog)
    validate_encounters(encounters, "fishing_mons", water_species, catalog)

    land_unique = [
        len({mon["species"] for mon in encounter["land_mons"]["mons"]})
        for encounter in encounters if encounter.get("land_mons") is not None
    ]

    ENCOUNTERS_PATH.write_text(json.dumps(payload, indent=2) + "\n", encoding="utf-8")
    print(f"Species covered by land encounters: {len(all_species)}")
    print(f"Water species assigned to surf/fishing: {len(water_species)}")
    print(f"Updated encounter slots: {slot_counts}")
    print(f"Distinct land species per table: {min(land_unique)}-{max(land_unique)}")


if __name__ == "__main__":
    main()
