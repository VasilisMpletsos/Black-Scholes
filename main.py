from datetime import datetime

import pandas as pd

from black_scholes import BlackScholesModel

RISK_FREE_RATE = 0.01575
VOLATILITY = 0.25
RUNS = 100

if __name__ == "__main__":
    df = pd.read_csv("./datasets/option_data.csv")
    options_prices = df["Option Price"].values
    strike_prices = df["Strike Price"].values
    tte_prices = df["Time to Expiration"].values
    option_types = df["Option Type"].values

    dataset_size = len(options_prices)

    start_time = datetime.now()
    for _ in range(RUNS):
        for i in range(dataset_size):
            bsm = BlackScholesModel(
                S=options_prices[i],
                K=strike_prices[i],
                T=tte_prices[i],
                r=RISK_FREE_RATE,
                sigma=VOLATILITY,
            )
            if option_types[i]:
                predicted_price = bsm.call_option_price()
                # print(f"Call Option Price: {predicted_price}")
            else:
                predicted_price = bsm.put_option_price()
                # print(f"Put Option Price: {predicted_price}")

    end_time = datetime.now()
    elapsed = end_time - start_time

    # Get total elapsed time in different units
    total_seconds = elapsed.total_seconds()
    total_milliseconds = total_seconds * 1000
    total_microseconds = elapsed.total_seconds() * 1_000_000

    print(f"\nTime taken: {total_milliseconds:.2f} ms")
    print(f"Time taken: {total_seconds:.6f} seconds")
    print(
        f"Average time per option: {total_milliseconds / (RUNS * dataset_size):.6f} ms"
    )
