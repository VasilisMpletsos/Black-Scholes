from datetime import datetime

import matplotlib.pyplot as plt
import numpy as np
import pandas as pd
import scipy.stats as si
import seaborn as sns

from black_scholes import BlackScholesModel

RISK_FREE_RATE = 0.01575
VOLATILITY = 0.25

if __name__ == "__main__":
    df = pd.read_csv("./datasets/option_data.csv")
    options_prices = df["Option Price"].values
    strike_prices = df["Strike Price"].values
    tte_prices = df["Time to Expiration"].values
    option_types = df["Option Type"].values

    dataset_size = len(options_prices)

    start_time = datetime.now()
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
    print(f"\nTime taken: {(end_time - start_time).microseconds / 1000} ms")
