import json
import matplotlib.pyplot as plt
import numpy as np
import seaborn as sns
import sys

# Load the JSON data
with open('build/lcr_simulation_results.json', 'r') as f:
    data = json.load(f)

# Select a game to visualize (e.g., the first game)
game_index = int(sys.argv[1]) if len(sys.argv) > 1 else 0
game = data[game_index]

# Check if chipHistory exists
if 'chipHistory' not in game:
    print("No chip history found in the data")
    exit(1)

# Extract chip history
chip_history = np.array(game['chipHistory'])

# Create heatmap
plt.figure(figsize=(12, 8))
sns.heatmap(
    chip_history.T,  # Transpose to make players rows and turns columns
    cmap='YlOrRd',
    cbar_kws={'label': 'Chips'},
    linewidths=0.5
)

plt.title(f"Chip Distribution - Game {game['gameId']}")
plt.xlabel("Turn")
plt.ylabel("Player")
plt.tight_layout()
plt.savefig(f"heatmap_game_{game['gameId']}.png")
plt.show()