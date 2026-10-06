#!/usr/bin/env python3
"""
plot_shielding_comparison.py

Reads CSV outputs from batch_scan.py and generates publication-quality
figures comparing neutron shielding effectiveness across all of
Prof. Kai's alloy compositions.

Produces 4 figures:
  1. Energy-dependent neutron transmission T(E) — all materials
  2. Effective linear attenuation coefficient μ_eff(E)
  3. Bar chart: integrated transmission ratio comparison
  4. Secondary gamma production spectra

Usage:
    python3 plot_shielding_comparison.py [--results-dir results/]

Author: Nghia
"""
import os, sys, glob, argparse
import numpy as np

try:
    import matplotlib
    matplotlib.use('Agg')  # non-interactive backend
    import matplotlib.pyplot as plt
    from matplotlib.ticker import LogLocator, LogFormatterSciNotation
    HAS_MPL = True
except ImportError:
    HAS_MPL = False
    print("WARNING: matplotlib not found. Install with: pip install matplotlib")
    print("         Skipping plot generation.")

# ================================================================
# Color scheme: distinctive, colorblind-friendly
# ================================================================
COLORS = {
    'SS316L':              '#555555',   # gray (baseline)
    'Pure_Fe':             '#888888',   # light gray
    'FeCoNiCr':            '#1f77b4',   # blue
    'Fe30Ni30Cr30Al8Ta2':  '#d62728',   # red (KEY)
    'Fe30Ni30Cr30Al10':    '#ff7f0e',   # orange
    'L12_HEA_13elem':      '#2ca02c',   # green (KEY)
    'CCIMA_CoNiAlTiTaNb':  '#9467bd',   # purple (KEY)
    'FeCoNiCrAl0.3':       '#8c564b',   # brown
    'Inconel718':          '#e377c2',   # pink
}

LINESTYLES = {
    'SS316L':              '--',
    'Pure_Fe':             ':',
    'FeCoNiCr':            '-',
    'Fe30Ni30Cr30Al8Ta2':  '-',
    'Fe30Ni30Cr30Al10':    '-.',
    'L12_HEA_13elem':      '-',
    'CCIMA_CoNiAlTiTaNb':  '-',
    'FeCoNiCrAl0.3':       '-.',
    'Inconel718':          '--',
}

LABELS = {
    'SS316L':              'SS316L (baseline)',
    'Pure_Fe':             'Pure Fe',
    'FeCoNiCr':            'FeCoNiCr',
    'Fe30Ni30Cr30Al8Ta2':  'Fe₃₀Ni₃₀Cr₃₀Al₈Ta₂ (2Ta-HEA)',
    'Fe30Ni30Cr30Al10':    'Fe₃₀Ni₃₀Cr₃₀Al₁₀ (0Ta-HEA)',
    'L12_HEA_13elem':      'L1₂-HEA (13-element)',
    'CCIMA_CoNiAlTiTaNb':  'CCIMA CoNiAlTiTaNb',
    'FeCoNiCrAl0.3':       'FeCoNiCrAl₀.₃',
    'Inconel718':          'Inconel 718',
}


def load_spectrum(filepath):
    """Load a neutron or gamma transmission CSV file."""
    data = np.loadtxt(filepath, delimiter=',', comments='#')
    # Columns: E_low, E_high, E_center, transmitted, incident, ratio
    return {
        'E_low':    data[:, 0],
        'E_high':   data[:, 1],
        'E_center': data[:, 2],
        'trans':    data[:, 3],
        'inc':      data[:, 4] if data.shape[1] > 4 else None,
        'ratio':    data[:, 5] if data.shape[1] > 5 else data[:, 3],
    }


def smooth(y, window=5):
    """Simple moving average for noisy MC data."""
    if len(y) < window:
        return y
    kernel = np.ones(window) / window
    return np.convolve(y, kernel, mode='same')


def plot_transmission_spectra(results_dir, thickness_mm):
    """Fig 1: Energy-dependent neutron transmission T(E)."""
    fig, ax = plt.subplots(figsize=(10, 6))

    files = sorted(glob.glob(os.path.join(results_dir, "neutron_*.csv")))
    for fpath in files:
        label_key = os.path.basename(fpath).replace("neutron_", "").replace(".csv", "")
        if label_key not in LABELS:
            continue
        data = load_spectrum(fpath)
        # Only plot bins with statistics
        mask = data['inc'] > 5  # at least 5 incident neutrons in bin
        if not np.any(mask):
            continue
        E = data['E_center'][mask]
        T = data['ratio'][mask]
        T_smooth = smooth(T, window=7)

        ax.plot(E, T_smooth,
                color=COLORS.get(label_key, 'gray'),
                ls=LINESTYLES.get(label_key, '-'),
                lw=2 if 'Ta2' in label_key or 'L12' in label_key or 'CCIMA' in label_key else 1.5,
                label=LABELS.get(label_key, label_key),
                alpha=0.85)

    ax.set_xscale('log')
    ax.set_yscale('log')
    ax.set_xlabel('Neutron Energy (MeV)', fontsize=13)
    ax.set_ylabel('Transmission Ratio T(E)', fontsize=13)
    ax.set_title(f'Neutron Transmission Through {thickness_mm} mm Slab\n'
                 f'(²³⁵U Fission Spectrum Source)', fontsize=14)
    ax.set_xlim(1e-8, 20)
    ax.set_ylim(1e-4, 2)
    ax.axhline(y=1, color='gray', ls=':', lw=0.5)
    ax.legend(fontsize=9, loc='upper right', framealpha=0.9)
    ax.grid(True, which='major', alpha=0.3)
    ax.grid(True, which='minor', alpha=0.1)

    plt.tight_layout()
    outfile = os.path.join(results_dir, "fig1_transmission_spectra.png")
    fig.savefig(outfile, dpi=300, bbox_inches='tight')
    print(f"  → {outfile}")
    plt.close(fig)


def plot_attenuation_coefficient(results_dir, thickness_mm):
    """Fig 2: Effective linear attenuation coefficient μ(E)."""
    d_cm = thickness_mm / 10.0
    fig, ax = plt.subplots(figsize=(10, 6))

    files = sorted(glob.glob(os.path.join(results_dir, "neutron_*.csv")))
    for fpath in files:
        label_key = os.path.basename(fpath).replace("neutron_", "").replace(".csv", "")
        if label_key not in LABELS:
            continue
        data = load_spectrum(fpath)
        mask = (data['inc'] > 10) & (data['ratio'] > 0) & (data['ratio'] < 1)
        if not np.any(mask):
            continue
        E = data['E_center'][mask]
        mu = -np.log(data['ratio'][mask]) / d_cm
        mu_smooth = smooth(mu, window=7)

        ax.plot(E, mu_smooth,
                color=COLORS.get(label_key, 'gray'),
                ls=LINESTYLES.get(label_key, '-'),
                lw=2 if 'Ta2' in label_key or 'L12' in label_key or 'CCIMA' in label_key else 1.5,
                label=LABELS.get(label_key, label_key),
                alpha=0.85)

    ax.set_xscale('log')
    ax.set_yscale('log')
    ax.set_xlabel('Neutron Energy (MeV)', fontsize=13)
    ax.set_ylabel('μ_eff (cm⁻¹)', fontsize=13)
    ax.set_title(f'Effective Linear Attenuation Coefficient\n'
                 f'(derived from {thickness_mm} mm slab, fission spectrum)', fontsize=14)
    ax.set_xlim(1e-8, 20)
    ax.legend(fontsize=9, loc='upper right', framealpha=0.9)
    ax.grid(True, which='major', alpha=0.3)

    plt.tight_layout()
    outfile = os.path.join(results_dir, "fig2_attenuation_coefficient.png")
    fig.savefig(outfile, dpi=300, bbox_inches='tight')
    print(f"  → {outfile}")
    plt.close(fig)


def plot_integrated_comparison(results_dir):
    """Fig 3: Bar chart of integrated transmission ratio."""
    summary_file = os.path.join(results_dir, "summary.csv")
    if not os.path.exists(summary_file):
        print("  WARNING: summary.csv not found, skipping bar chart")
        return

    materials = []; ratios = []
    with open(summary_file) as f:
        next(f)  # skip header
        for line in f:
            parts = line.strip().split(',')
            if len(parts) >= 4:
                materials.append(parts[0])
                ratios.append(float(parts[3]))

    # Sort by ratio (best shielding = lowest ratio first)
    pairs = sorted(zip(materials, ratios), key=lambda x: x[1])
    materials = [p[0] for p in pairs]
    ratios = [p[1] for p in pairs]

    fig, ax = plt.subplots(figsize=(10, 6))
    colors = [COLORS.get(m, '#aaaaaa') for m in materials]
    labels = [LABELS.get(m, m) for m in materials]

    bars = ax.barh(range(len(materials)), ratios, color=colors, edgecolor='white', height=0.7)

    ax.set_yticks(range(len(materials)))
    ax.set_yticklabels(labels, fontsize=10)
    ax.set_xlabel('Integrated Neutron Transmission Ratio', fontsize=13)
    ax.set_title('Neutron Shielding Comparison — All Materials\n'
                 '(lower = better shielding)', fontsize=14)
    ax.invert_yaxis()

    # Add value labels on bars
    for bar, ratio in zip(bars, ratios):
        ax.text(bar.get_width() + 0.005, bar.get_y() + bar.get_height()/2,
                f'{ratio:.4f}', va='center', fontsize=9, color='#333333')

    # Highlight the SS316L baseline
    ss_idx = next((i for i, m in enumerate(materials) if 'SS316L' in m), None)
    if ss_idx is not None:
        ax.axvline(x=ratios[ss_idx], color='#555555', ls='--', lw=1,
                   label=f'SS316L baseline: {ratios[ss_idx]:.4f}')
        ax.legend(fontsize=10)

    ax.grid(True, axis='x', alpha=0.3)
    plt.tight_layout()
    outfile = os.path.join(results_dir, "fig3_integrated_comparison.png")
    fig.savefig(outfile, dpi=300, bbox_inches='tight')
    print(f"  → {outfile}")
    plt.close(fig)


def plot_secondary_gammas(results_dir):
    """Fig 4: Secondary gamma production spectra."""
    fig, ax = plt.subplots(figsize=(10, 6))

    files = sorted(glob.glob(os.path.join(results_dir, "gamma_*.csv")))
    for fpath in files:
        label_key = os.path.basename(fpath).replace("gamma_", "").replace(".csv", "")
        if label_key not in LABELS:
            continue
        data = np.loadtxt(fpath, delimiter=',', comments='#')
        if data.shape[1] < 4:
            continue
        E = data[:, 2]  # E_center
        counts = data[:, 3]
        mask = counts > 0
        if not np.any(mask):
            continue
        counts_smooth = smooth(counts[mask], window=5)

        ax.plot(E[mask], counts_smooth,
                color=COLORS.get(label_key, 'gray'),
                ls=LINESTYLES.get(label_key, '-'),
                lw=1.5,
                label=LABELS.get(label_key, label_key),
                alpha=0.8)

    ax.set_xscale('log')
    ax.set_yscale('log')
    ax.set_xlabel('Gamma Energy (MeV)', fontsize=13)
    ax.set_ylabel('Transmitted Gamma Count', fontsize=13)
    ax.set_title('Secondary Gamma Production & Transmission\n'
                 '(from neutron capture and inelastic scattering)', fontsize=14)
    ax.legend(fontsize=9, loc='upper right', framealpha=0.9)
    ax.grid(True, which='major', alpha=0.3)

    plt.tight_layout()
    outfile = os.path.join(results_dir, "fig4_secondary_gammas.png")
    fig.savefig(outfile, dpi=300, bbox_inches='tight')
    print(f"  → {outfile}")
    plt.close(fig)


def main():
    parser = argparse.ArgumentParser(
        description="Plot shielding comparison from KaiAlloyShielding results")
    parser.add_argument("--results-dir", default="results/",
                        help="Directory containing CSV outputs")
    parser.add_argument("--thickness", type=float, default=20.0,
                        help="Shield thickness in mm (for μ calculation)")
    args = parser.parse_args()

    if not HAS_MPL:
        sys.exit(1)

    print(f"\nGenerating figures from {args.results_dir}...")
    print(f"Shield thickness: {args.thickness} mm\n")

    # Set global style
    plt.rcParams.update({
        'font.family': 'sans-serif',
        'font.sans-serif': ['Arial', 'DejaVu Sans'],
        'font.size': 11,
        'axes.linewidth': 1.2,
        'xtick.major.width': 1.0,
        'ytick.major.width': 1.0,
    })

    plot_transmission_spectra(args.results_dir, args.thickness)
    plot_attenuation_coefficient(args.results_dir, args.thickness)
    plot_integrated_comparison(args.results_dir)
    plot_secondary_gammas(args.results_dir)

    print(f"\nAll figures saved to {args.results_dir}")
    print("These can be directly included in the proof-of-concept attachment.")


if __name__ == "__main__":
    main()
