#!/usr/bin/env python3
"""
review.py — Daily proof review picker.

Loads cards.json and selects a random theorem/lemma/proposition
for proof practice. Filters to proof_sketch cards by default.

Usage:
    python scripts/review.py                    # random from all decks
    python scripts/review.py Sequences          # random from one deck
    python scripts/review.py Sequences Bounding # random from multiple decks
    python scripts/review.py --list             # show available decks
    python scripts/review.py --all-types        # include statement + uses cards too
    python scripts/review.py -n 3              # pick 3 theorems
"""

import json
import random
import argparse
import sys
from pathlib import Path

CARDS_JSON = Path(__file__).parent / 'cards.json'

# Card types considered worth proving from scratch
PROOF_TYPES = {'proof_sketch'}
ALL_REVIEW_TYPES = {'proof_sketch', 'statement', 'uses'}


def load_cards(decks_filter=None, card_types=PROOF_TYPES):
    with open(CARDS_JSON) as f:
        data = json.load(f)

    decks = data['decks']

    if decks_filter:
        # Case-insensitive partial match
        matched = []
        for name in decks:
            if any(d.lower() in name.lower() for d in decks_filter):
                matched.append(name)
        if not matched:
            print(f"No decks matched: {decks_filter}")
            print(f"Available: {list(decks.keys())}")
            sys.exit(1)
        selected_decks = {k: decks[k] for k in matched}
    else:
        selected_decks = decks

    cards = [
        c for deck in selected_decks.values()
        for c in deck
        if c['card_type'] in card_types
        and c['type'] in {'Theorem', 'Proposition', 'Lemma', 'Corollary'}
    ]

    return cards, list(selected_decks.keys())


def format_card(card):
    lines = [
        f"  Type:   {card['type']}",
        f"  Name:   {card['name']}",
        f"  Deck:   {card['deck']}",
        f"  Label:  {card['label']}",
    ]
    return '\n'.join(lines)


def main():
    parser = argparse.ArgumentParser(
        description='Pick a random theorem for daily proof review.')
    parser.add_argument('decks', nargs='*',
                        help='Deck name(s) to draw from (default: all)')
    parser.add_argument('--list', action='store_true',
                        help='List available decks and card counts')
    parser.add_argument('--all-types', action='store_true',
                        help='Include statement and uses cards (not just proof_sketch)')
    parser.add_argument('-n', type=int, default=1,
                        help='Number of theorems to pick (default: 1)')
    args = parser.parse_args()

    # --list
    if args.list:
        with open(CARDS_JSON) as f:
            data = json.load(f)
        print("Available decks:\n")
        for name, cards in data['decks'].items():
            proof_count = sum(1 for c in cards
                              if c['card_type'] == 'proof_sketch'
                              and c['type'] in {'Theorem','Proposition','Lemma','Corollary'})
            total = len(cards)
            print(f"  {name:<30} {proof_count:>3} proof cards  ({total} total)")
        print(f"\n  Total cards: {data['total']}")
        return

    card_types = ALL_REVIEW_TYPES if args.all_types else PROOF_TYPES
    cards, deck_names = load_cards(args.decks or None, card_types)

    if not cards:
        print(f"No proof cards found in: {deck_names}")
        sys.exit(1)

    n = min(args.n, len(cards))
    picks = random.sample(cards, n)

    print(f"\n{'─'*50}")
    print(f"  Daily Proof Review — {n} theorem{'s' if n > 1 else ''}")
    print(f"  Decks: {', '.join(deck_names)}")
    print(f"{'─'*50}\n")

    for i, card in enumerate(picks, 1):
        if n > 1:
            print(f"  [{i}]")
        print(format_card(card))
        if i < n:
            print()

    print(f"\n{'─'*50}")
    print("  Prove it from scratch. Then check your proof files.")
    print(f"{'─'*50}\n")


if __name__ == '__main__':
    main()
