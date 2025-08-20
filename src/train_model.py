#!/usr/bin/env python3
"""
Simple MACD + Volume Strategy - Test Version
Buy: MACD crosses above signal + volume spike
Sell: MACD crosses below signal OR after holding period
"""

import pandas as pd
import numpy as np
from sklearn.ensemble import RandomForestClassifier
from sklearn.metrics import classification_report
import requests

# AGGRESSIVE TEST PARAMETERS (to catch more signals)
STOP_LOSS_PCT = 0.03        # 3% stop loss (wider)
TAKE_PROFIT_PCT = 0.015     # 1.5% take profit (tighter)  
VOLUME_SPIKE = 1.1          # Volume just 10% above average (much lower)
MIN_HOLD_HOURS = 1          # Hold for at least 1 hour
MAX_HOLD_HOURS = 8          # Exit after 8 hours max

def get_binance_data(symbol="BTCUSDT", interval="1h", limit=1500):
    """Get historical data from Binance API"""
    url = "https://api.binance.com/api/v3/klines"
    params = {
        'symbol': symbol,
        'interval': interval,
        'limit': limit
    }
    
    print(f"Requesting data from: {url}")
    print(f"Parameters: {params}")
    
    try:
        response = requests.get(url, params=params)
        print(f"Response status: {response.status_code}")
        
        if response.status_code != 200:
            print(f"API Error: {response.text}")
            return pd.DataFrame()
        
        data = response.json()
        print(f"Received {len(data)} data points")
        
        if not data:
            print("Empty data received from API")
            return pd.DataFrame()
        
        columns = ['open_time', 'open', 'high', 'low', 'close', 'volume', 
                   'close_time', 'quote_volume', 'trades', 'taker_buy_base', 
                   'taker_buy_quote', 'ignore']
        
        df = pd.DataFrame(data, columns=columns)
        
        # Convert to numeric and check
        for col in ['open', 'high', 'low', 'close', 'volume']:
            df[col] = pd.to_numeric(df[col], errors='coerce')
        
        result = df[['open', 'high', 'low', 'close', 'volume']]
        print(f"Final DataFrame shape: {result.shape}")
        print(f"Sample data:")
        print(result.head())
        print(f"Price range: ${result['close'].min():.2f} - ${result['close'].max():.2f}")
        
        return result
        
    except Exception as e:
        print(f"Error downloading data: {e}")
        return pd.DataFrame()

def calculate_simple_indicators(df):
    """Calculate basic indicators"""
    close = df['close']
    volume = df['volume']
    
    # MACD (12, 26, 9)
    ema_12 = close.ewm(span=12).mean()
    ema_26 = close.ewm(span=26).mean()
    macd = ema_12 - ema_26
    signal = macd.ewm(span=9).mean()
    histogram = macd - signal
    
    # Simple crossovers
    macd_bullish = ((macd > signal) & (macd.shift(1) <= signal.shift(1)))
    macd_bearish = ((macd < signal) & (macd.shift(1) >= signal.shift(1)))
    
    # Volume
    volume_ma = volume.rolling(20).mean()
    volume_spike = volume > (volume_ma * VOLUME_SPIKE)
    
    # DEBUG: Count crossovers
    bullish_count = macd_bullish.sum()
    bearish_count = macd_bearish.sum()
    volume_spikes = volume_spike.sum()
    
    print(f"DEBUG: Found {bullish_count} bullish MACD crossovers")
    print(f"DEBUG: Found {bearish_count} bearish MACD crossovers") 
    print(f"DEBUG: Found {volume_spikes} volume spikes")
    print(f"DEBUG: Data range: {len(df)} periods")
    print(f"DEBUG: MACD range: {macd.min():.4f} to {macd.max():.4f}")
    print(f"DEBUG: Signal range: {signal.min():.4f} to {signal.max():.4f}")
    
    return pd.DataFrame({
        'close': close,
        'volume': volume,
        'macd': macd,
        'signal': signal,
        'histogram': histogram,
        'macd_bullish': macd_bullish,
        'macd_bearish': macd_bearish,
        'volume_spike': volume_spike,
        'volume_ratio': volume / volume_ma
    })

def simple_backtest(df):
    """Simple backtest - no ML, just rule-based"""
    indicators = calculate_simple_indicators(df)
    
    trades = []
    position = None
    entry_price = 0
    entry_time = 0
    
    print("Running simple backtest...")
    
    for i in range(30, len(df)):  # Start after indicators stabilize
        current_price = indicators['close'].iloc[i]
        
        # DEBUG: Print first few potential signals
        if i < 35 and indicators['macd_bullish'].iloc[i]:
            print(f"DEBUG: Potential BUY at {i}: MACD={indicators['macd'].iloc[i]:.4f}, Signal={indicators['signal'].iloc[i]:.4f}, Vol ratio={indicators['volume_ratio'].iloc[i]:.2f}")
        
        # ENTRY: MACD bullish cross + any volume increase
        if (indicators['macd_bullish'].iloc[i] and 
            indicators['volume_ratio'].iloc[i] > VOLUME_SPIKE and 
            position is None):
            
            position = 'LONG'
            entry_price = current_price
            entry_time = i
            print(f"BUY at index {i}: Price ${current_price:.2f}, MACD {indicators['macd'].iloc[i]:.4f}, Volume ratio {indicators['volume_ratio'].iloc[i]:.2f}")
        
        # EXIT CONDITIONS
        elif position == 'LONG':
            hours_held = i - entry_time
            price_change = (current_price - entry_price) / entry_price
            
            exit_reason = None
            
            # Take profit
            if price_change >= TAKE_PROFIT_PCT:
                exit_reason = "TAKE_PROFIT"
            
            # Stop loss
            elif price_change <= -STOP_LOSS_PCT:
                exit_reason = "STOP_LOSS"
            
            # MACD bearish cross (after minimum hold)
            elif (hours_held >= MIN_HOLD_HOURS and 
                  indicators['macd_bearish'].iloc[i]):
                exit_reason = "MACD_BEARISH"
            
            # Maximum hold time
            elif hours_held >= MAX_HOLD_HOURS:
                exit_reason = "MAX_HOLD"
            
            if exit_reason:
                profit_pct = price_change * 100
                trades.append({
                    'entry_index': entry_time,
                    'exit_index': i,
                    'entry_price': entry_price,
                    'exit_price': current_price,
                    'profit_pct': profit_pct,
                    'hours_held': hours_held,
                    'exit_reason': exit_reason
                })
                
                print(f"SELL at index {i}: Price ${current_price:.2f}, Profit {profit_pct:.2f}%, Reason: {exit_reason}")
                position = None
                entry_price = 0
                entry_time = 0
    
    return trades, indicators

def analyze_backtest_results(trades):
    """Analyze backtest performance"""
    if not trades:
        print("No trades generated!")
        return
    
    df_trades = pd.DataFrame(trades)
    
    total_trades = len(trades)
    winning_trades = len(df_trades[df_trades['profit_pct'] > 0])
    losing_trades = len(df_trades[df_trades['profit_pct'] < 0])
    
    total_return = df_trades['profit_pct'].sum()
    avg_return = df_trades['profit_pct'].mean()
    best_trade = df_trades['profit_pct'].max()
    worst_trade = df_trades['profit_pct'].min()
    
    win_rate = (winning_trades / total_trades) * 100
    
    print(f"\n=== BACKTEST RESULTS ===")
    print(f"Total trades: {total_trades}")
    print(f"Winning trades: {winning_trades}")
    print(f"Losing trades: {losing_trades}")
    print(f"Win rate: {win_rate:.1f}%")
    print(f"Total return: {total_return:.2f}%")
    print(f"Average return per trade: {avg_return:.2f}%")
    print(f"Best trade: {best_trade:.2f}%")
    print(f"Worst trade: {worst_trade:.2f}%")
    
    # Exit reason analysis
    exit_reasons = df_trades['exit_reason'].value_counts()
    print(f"\nExit reasons:")
    for reason, count in exit_reasons.items():
        print(f"  {reason}: {count}")
    
    # Show some example trades
    print(f"\nFirst 5 trades:")
    print(df_trades[['entry_price', 'exit_price', 'profit_pct', 'hours_held', 'exit_reason']].head())

def create_ml_features(indicators):
    """Create features for ML model"""
    features = pd.DataFrame()
    
    # Current state
    features['macd_above_signal'] = (indicators['macd'] > indicators['signal']).astype(int)
    features['macd_bullish'] = indicators['macd_bullish'].astype(int)
    features['macd_bearish'] = indicators['macd_bearish'].astype(int)
    features['volume_spike'] = indicators['volume_spike'].astype(int)
    features['volume_ratio'] = indicators['volume_ratio']
    
    # MACD strength
    features['macd_signal_diff'] = indicators['macd'] - indicators['signal']
    features['histogram'] = indicators['histogram']
    
    # Trends
    features['macd_trend'] = (indicators['macd'] > indicators['macd'].shift(3)).astype(int)
    features['price_trend'] = (indicators['close'] > indicators['close'].shift(3)).astype(int)
    
    # Volatility
    features['price_volatility'] = indicators['close'].rolling(10).std() / indicators['close'].rolling(10).mean()
    
    return features

def create_ml_targets(trades, total_length):
    """Create targets from backtest results"""
    targets = pd.Series(0, index=range(total_length))  # 0 = HOLD
    
    for trade in trades:
        targets.iloc[trade['entry_index']] = 1  # BUY signal
        targets.iloc[trade['exit_index']] = 2   # SELL signal
    
    return targets

def train_simple_model():
    print("Downloading Bitcoin data...")
    df = get_binance_data("BTCUSDT", "1h", 1000)
    
    print("Running simple strategy backtest...")
    trades, indicators = simple_backtest(df)
    
    analyze_backtest_results(trades)
    
    if not trades:
        print("No trades to learn from. Adjusting parameters...")
        return None, None
    
    print(f"\nCreating ML model from {len(trades)} trades...")
    
    # Create features and targets
    features = create_ml_features(indicators)
    targets = create_ml_targets(trades, len(df))
    
    # Clean data
    valid_start = 30
    X = features.iloc[valid_start:]
    y = targets.iloc[valid_start:]
    
    # Remove NaN
    mask = ~X.isna().any(axis=1)
    X_clean = X[mask]
    y_clean = y[mask]
    
    if len(X_clean) == 0:
        print("No clean training data!")
        return None, None
    
    print(f"Training with {len(X_clean)} samples")
    
    # Simple train/test split
    split_idx = int(len(X_clean) * 0.8)
    X_train, X_test = X_clean.iloc[:split_idx], X_clean.iloc[split_idx:]
    y_train, y_test = y_clean.iloc[:split_idx], y_clean.iloc[split_idx:]
    
    # Train model
    model = RandomForestClassifier(
        n_estimators=50,
        max_depth=6,
        random_state=42,
        class_weight='balanced'
    )
    model.fit(X_train, y_train)
    
    # Test
    test_pred = model.predict(X_test)
    print(f"\nML Model Performance:")
    print(classification_report(y_test, test_pred, target_names=['HOLD', 'BUY', 'SELL'], zero_division=0))
    
    # Feature importance
    importance = pd.DataFrame({
        'feature': X.columns,
        'importance': model.feature_importances_
    }).sort_values('importance', ascending=False)
    
    print(f"\nTop 5 Important Features:")
    print(importance.head())
    
    # Save model
    import pickle
    model_data = {
        'model': model,
        'feature_names': list(X.columns),
        'backtest_trades': len(trades),
        'backtest_return': sum(trade['profit_pct'] for trade in trades),
        'parameters': {
            'stop_loss': STOP_LOSS_PCT,
            'take_profit': TAKE_PROFIT_PCT,
            'volume_spike': VOLUME_SPIKE,
            'min_hold': MIN_HOLD_HOURS,
            'max_hold': MAX_HOLD_HOURS
        }
    }
    
    with open('simple_trading_model.pkl', 'wb') as f:
        pickle.dump(model_data, f)
    
    print(f"\nModel saved! Based on backtest with {len(trades)} trades")
    
    return model, trades

if __name__ == "__main__":
    model, trades = train_simple_model()
    if model is not None:
        print("\nSimple strategy training complete!")
    else:
        print("\nStrategy needs adjustment - no profitable patterns found.")