# SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

import argparse
import pandas as pd

if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        description="Load JSON and generate statistics using Pandas."
    )
    parser.add_argument("json_file", type=str, help="Path to the input JSON file.")
    parser.add_argument(
        "--output",
        type=str,
        default="design_stats.png",
        help="Optional: Save computed statistics to this JSON file.",
    )
    args = parser.parse_args()

    df = pd.read_json(args.json_file)
    pandas_data = pd.DataFrame(df.set_index("Name"))
    plot = pandas_data.plot.bar(y=["terms", "nets", "instances"], stacked=True)

    # Customize plot
    plot.set_title("Design Statistics", fontsize=16, fontweight="bold")
    plot.set_xlabel("Design Name", fontsize=12)
    plot.set_ylabel("Count", fontsize=12)

    plot_figure = plot.get_figure()
    plot_figure.tight_layout()
    plot_figure.savefig(args.output)
