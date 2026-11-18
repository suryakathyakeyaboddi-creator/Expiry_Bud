# Requirements

## Product fields

- Persistent unique ID
- Name and category
- Optional manufacture date
- Required expiry date
- Positive quantity and unit
- Optional non-negative unit price
- Date added
- Inventory outcome and optional outcome date

## Functional requirements

1. Register, update, list, search, filter, sort, and delete active products.
2. Supply default categories and allow custom categories.
3. Reject an expiry date that is not after a supplied manufacture date.
4. Calculate status from today's date and the configured threshold.
5. Treat expiry today as Expiring.
6. Display alerts when zero through threshold days remain.
7. Move overdue active products to history as ExpiredUnused.
8. Allow users to archive active products as Consumed or Discarded.
9. Keep active products and history in separate CSV files without duplicate IDs.
10. Report waste counts, waste rate, estimated loss, category totals, monthly totals, and the most-wasted category.
11. Persist categories and settings between sessions.
12. Report readable input and file errors without losing existing records.
13. Provide a responsive browser dashboard for all Version 1 operations.
14. Keep dashboard and terminal data compatible through the shared CSV schema.
15. Validate every dashboard mutation in the API before writing files.

## Version 1 constraints

- C++17 with no external libraries
- Console interface
- One household and no authentication
- CSV persistence
- Alerts operate only while the program is running
- Dashboard server binds to the local machine and does not provide authentication
