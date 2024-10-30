import matplotlib.pyplot as plt
import numpy as np
import scipy.stats as si


class BlackScholesModel:
    def __init__(self, S, K, T, r, sigma):
        self.S = S  # Underlying asset price
        self.K = K  # Option strike price
        self.T = T  # Time to expiration in years
        self.r = r  # Risk-free interest rate
        self.sigma = sigma  # Volatility of the underlying asset

    def d1(self):
        return (np.log(self.S / self.K) + (self.r + 0.5 * self.sigma**2) * self.T) / (
            self.sigma * np.sqrt(self.T)
        )

    def d2(self):
        return self.d1() - self.sigma * np.sqrt(self.T)

    def call_option_price(self):
        return self.S * si.norm.cdf(self.d1(), 0.0, 1.0) - self.K * np.exp(
            -self.r * self.T
        ) * si.norm.cdf(self.d2(), 0.0, 1.0)

    def put_option_price(self):
        return self.K * np.exp(-self.r * self.T) * si.norm.cdf(
            -self.d2(), 0.0, 1.0
        ) - self.S * si.norm.cdf(-self.d1(), 0.0, 1.0)


class BlackScholesGreeks(BlackScholesModel):
    def delta_call(self):
        return si.norm.cdf(self.d1(), 0.0, 1.0)

    def delta_put(self):
        return -si.norm.cdf(-self.d1(), 0.0, 1.0)

    def gamma(self):
        return si.norm.pdf(self.d1(), 0.0, 1.0) / (
            self.S * self.sigma * np.sqrt(self.T)
        )

    def theta_call(self):
        return -self.S * si.norm.pdf(self.d1(), 0.0, 1.0) * self.sigma / (
            2 * np.sqrt(self.T)
        ) - self.r * self.K * np.exp(-self.r * self.T) * si.norm.cdf(
            self.d2(), 0.0, 1.0
        )

    def theta_put(self):
        return -self.S * si.norm.pdf(self.d1(), 0.0, 1.0) * self.sigma / (
            2 * np.sqrt(self.T)
        ) + self.r * self.K * np.exp(-self.r * self.T) * si.norm.cdf(
            -self.d2(), 0.0, 1.0
        )

    def vega(self):
        return self.S * si.norm.pdf(self.d1(), 0.0, 1.0) * np.sqrt(self.T)

    def rho_call(self):
        return (
            self.K
            * self.T
            * np.exp(-self.r * self.T)
            * si.norm.cdf(self.d2(), 0.0, 1.0)
        )

    def rho_put(self):
        return (
            -self.K
            * self.T
            * np.exp(-self.r * self.T)
            * si.norm.cdf(-self.d2(), 0.0, 1.0)
        )


def calculate_historical_volatility(stock_data, window=252):
    log_returns = np.log(stock_data["Close"] / stock_data["Close"].shift(1))
    volatility = np.sqrt(window) * log_returns.std()
    return volatility
