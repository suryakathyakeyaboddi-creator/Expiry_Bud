# Architecture

```text
Application console menu
        |
        +-- ProductService
        +-- ExpiryService
        +-- SearchService
        +-- HistoryService
        +-- AnalyticsService
        |
     AppState
        |
     IDataStore
        |
   CsvDataStore
```

`Product` is the central domain model. Expiry status is derived from the current date. Inventory outcome is stored because it records a completed lifecycle event.

`IDataStore` and `IAlertService` demonstrate runtime polymorphism. `CsvDataStore` and `ConsoleAlertService` provide the Version 1 implementations. This keeps storage and notification choices separate from business rules.

## Dashboard architecture

```text
React + Bootstrap 5
        |
   Express JSON API
        |
Shared CSV data directory
        |
C++ CsvDataStore
```

The Express API reloads CSV state for each request so terminal changes appear after dashboard refresh. Mutating API operations are serialized and use atomic file replacement. The browser receives calculated status, alerts, counts, and analytics from the API rather than duplicating business rules in React.

The dashboard includes Overview, Products, History, Analytics, and Settings sections. Product actions cover registration, update, consumption, discard, and deletion. Category and alert-threshold changes persist to the same files used by C++.

## Startup sequence

1. Load products, history, categories, and settings.
2. Create default files when they do not exist.
3. Archive overdue active products once.
4. Save changed state atomically.
5. Display near-expiry alerts.
6. Open the main menu.

## Waste definition

Consumed products do not count as waste. Discarded and ExpiredUnused products count as waste. Estimated loss equals remaining quantity multiplied by unit price. Records without a price contribute to counts but not financial totals.
