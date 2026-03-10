import pandas as pd
import matplotlib.pyplot as plt
import numpy as np
import sys
import os

try:
    from scipy.signal import savgol_filter
    SCIPY_AVAILABLE = True
except ImportError:
    SCIPY_AVAILABLE = False

def format_label_with_unit(col_name):
    # Automatically add units from column name (e.g., Speed_km/h -> Speed [km/h])
    if '_' in col_name:
        parts = col_name.rsplit('_', 1)
        return f"{parts[0]} [{parts[1]}]"
    return col_name

def main():
    if len(sys.argv) < 2:
        print("Usage: python3 telemetry.py <path_to_csv_file>")
        sys.exit(1)

    file_path = sys.argv[1]

    if not os.path.isfile(file_path):
        print(f"Error: File '{file_path}' does not exist.")
        sys.exit(1)

    # Decrease default font size
    plt.rcParams.update({'font.size': 10})

    try:
        # 1. Load data
        df = pd.read_csv(file_path, sep=r'[;,\s]+', engine='python')
        
        # 2. Find time column
        time_col = next((col for col in df.columns if 'Time' in col or 'time' in col), None)
        if not time_col:
            print("Error: Time column not found.")
            sys.exit(1)

        speed_col = next((col for col in df.columns if 'Speed' in col or 'speed' in col), None)
        accel_col = next((col for col in df.columns if 'Accel' in col or 'accel' in col), None)

        # 3. Calculate ACCELERATION FROM GPS
        if speed_col:
            is_ms = 'ms' in time_col.lower()
            t_sec = df[time_col] / 1000.0 if is_ms else df[time_col]
            v_mps = df[speed_col] / 3.6
            
            delta_v = v_mps.diff(10)
            delta_t = t_sec.diff(10).replace(0, 0.001)
            
            df['Accel_GPS'] = delta_v / delta_t
            df['Accel_GPS'] = df['Accel_GPS'].rolling(window=15, min_periods=1, center=True).mean()

        # 4. Select columns for main axes (excluding Accel_GPS)
        y_cols = [col for col in df.columns if col not in [time_col, 'Accel_GPS'] and not df[col].isnull().all()]

        fig, axes = plt.subplots(nrows=len(y_cols), ncols=1, figsize=(14, 3.5 * len(y_cols)), sharex=True)
        
        if len(y_cols) == 1:
            axes = [axes]

        for i, col in enumerate(y_cols):
            ax = axes[i]
            
            # Smooth data (if possible), otherwise use raw data
            if SCIPY_AVAILABLE and len(df[col]) > 31:
                y_data = savgol_filter(df[col], window_length=31, polyorder=3)
            else:
                y_data = df[col].values

            # Set legend name and axis labels
            if col == accel_col:
                label_name = "Acc IMU [m/s2]"
                y_axis_label = "Accel [m/s2]"
            else:
                label_name = format_label_with_unit(col)
                y_axis_label = label_name

            # Draw main line
            ax.plot(df[time_col], y_data, color=f'C{i}', linewidth=2.5, label=label_name)
            
            # Find and draw peak for the main line
            max_idx = np.nanargmax(y_data)
            max_x = df[time_col].iloc[max_idx]
            max_y = y_data[max_idx]
            
            ax.annotate(f'Max: {max_y:.1f}', xy=(max_x, max_y), xytext=(0, 6),
                        textcoords='offset points', ha='center', va='bottom',
                        fontsize=10, fontweight='bold', color=f'C{i}',
                        bbox=dict(boxstyle='round,pad=0.2', fc='white', alpha=0.7, ec='none'))

            # GPS OVERLAY: If it's the acceleration axis, add GPS
            if col == accel_col and 'Accel_GPS' in df.columns:
                gps_data = df['Accel_GPS'].values
                
                ax.plot(df[time_col], gps_data, color='purple', linestyle='--', linewidth=2.5, label='Acc GPS [m/s2]')
                
                # Find and draw peak for GPS (safeguard against NaN)
                valid_gps_idx = np.where(~np.isnan(gps_data))[0]
                if len(valid_gps_idx) > 0:
                    max_gps_idx = valid_gps_idx[np.nanargmax(gps_data[valid_gps_idx])]
                    max_x_gps = df[time_col].iloc[max_gps_idx]
                    max_y_gps = gps_data[max_gps_idx]
                    
                    ax.annotate(f'Max: {max_y_gps:.1f}', xy=(max_x_gps, max_y_gps), xytext=(0, 6),
                                textcoords='offset points', ha='center', va='bottom',
                                fontsize=10, fontweight='bold', color='purple',
                                bbox=dict(boxstyle='round,pad=0.2', fc='white', alpha=0.7, ec='none'))

            # Format Y axis
            ax.set_ylabel(y_axis_label, fontweight='bold', fontsize=10)
            ax.grid(True, linestyle='--', alpha=0.7)
            ax.legend(loc='upper left', fontsize=10)

        # Format bottom X axis
        axes[-1].set_xlabel("Time [s]", fontweight='bold', fontsize=11)
        
        plt.tight_layout()
        plt.show()

    except Exception as e:
        print(f"An error occurred while processing the file:\n{e}")

if __name__ == '__main__':
    main()
