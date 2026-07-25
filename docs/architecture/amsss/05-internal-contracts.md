# Internal Contracts

Backend hooks never call public AMSSS allocation APIs. Engines use static state and bounded loops. Provider callbacks are synchronous and report actual bytes released. Heap corruption remains intentionally fail-stop.

