# Test cases

The automated test suite covers:

1. Valid dates, invalid dates, formatting, and leap years.
2. Day differences across month and year boundaries.
3. Expiry today, exact threshold, safe, and expired status rules.
4. Valid products and invalid quantity, category, and date combinations.
5. Persistent ID generation across active products and history.
6. Duplicate ID rejection against history.
7. Case-insensitive search and nearest-expiry sorting.
8. Automatic expiry archiving without duplicate records.
9. Consumed and discarded lifecycle transitions.
10. Waste counts, rate, estimated loss, category, and month calculations.
11. CSV round-trips for commas, quotes, optional dates, and prices.
12. Settings and custom-category persistence.
13. Corrupted CSV rejection with source line information.
14. Console alert wording for products expiring today.
15. Application startup, file creation, saving, and clean exit.
16. Protection against removing the final available category.
17. A complete scripted workflow covering all thirteen terminal menu options.

Run all tests with `make test`.

## Dashboard tests

Dashboard API tests under `tests/api` cover:

- Product creation, validation, update, search, filtering, outcomes, and deletion
- Missing records and malformed JSON
- Category creation, duplicates, used-category protection, and removal
- Alert-threshold persistence and invalid values
- Dashboard counts, expiry alerts, automatic archiving, and analytics
- Direct verification of CSV side effects

UI tests under `tests/ui` cover dashboard navigation and product-form normalization. Browser QA additionally exercises add, edit, consume, discard, history, analytics, categories, settings, search, and status filtering at desktop and mobile sizes.

Run all dashboard tests with `npm test`.
