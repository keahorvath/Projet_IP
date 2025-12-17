import matplotlib.pyplot as plt
import pandas as pd
import numpy as np

#This file contains all the methods I used to create the graphs needed for my report
#I used an LLM to help me write these functions faster because I was very late!

# Configuration pour un rendu "scientifique"
plt.rcParams['font.family'] = 'serif'
plt.rcParams['font.size'] = 11

def single_vs_multi():
    # 1. Lecture du fichier CSV
    try:
        file_path = "results/single_vs_multi.csv"
        # On utilise sep=";" car tes données sont séparées par des points-virgules
        df = pd.read_csv(file_path, sep=";")
    except FileNotFoundError:
        print(f"Erreur: Le fichier '{file_path}' est introuvable.")
        return

    # Nettoyage des noms de colonnes (suppression des espaces éventuels autour des noms)
    df.columns = df.columns.str.strip()

    # 2. Filtrage des données
    # On garde uniquement les lignes où 'SINGLE Value' ne contient PAS "(TLR)"
    # On s'assure d'abord que la colonne est traitée comme du texte (.astype(str))
    mask_solved = ~df['SINGLE Value'].astype(str).str.contains(r'\(TLR\)')
    df_clean = df[mask_solved].copy()

    if df_clean.empty:
        print("Attention: Aucune instance résolue à l'optimalité trouvée (toutes sont TLR).")
        return

    # Extraction des labels (ex: "uniform_12" -> "12" pour alléger l'axe X)
    # On suppose que le format est "prefix_Numero"
    labels = df_clean['Instance'].apply(lambda x: x.split('_')[-1] if '_' in x else x)
    x = np.arange(len(labels))  # Positions des barres
    width = 0.35  # Largeur des barres

    # 3. Création des Graphes
    fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(12, 5))

    # --- Graphe 1 : Nombre de Colonnes (Echelle Log) ---
    rects1 = ax1.bar(x - width/2, df_clean['Nb cols Single'], width, label='SINGLE', color='#4c72b0') # Bleu "seaborn"
    rects2 = ax1.bar(x + width/2, df_clean['Nb cols Multi'], width, label='MULTI', color='#c44e52') # Rouge "seaborn"

    ax1.set_xlabel('Instance ID')
    ax1.set_ylabel('Nombre de Colonnes (Log Scale)')
    ax1.set_title('Colonnes Générées')
    ax1.set_xticks(x)
    ax1.set_xticklabels(labels, rotation=45, ha='right')
    ax1.legend()
    ax1.set_yscale('log') # Echelle log car MULTI explose souvent le nb de colonnes
    ax1.grid(axis='y', linestyle='--', alpha=0.7)

    # --- Graphe 2 : Temps d'exécution ---
    rects3 = ax2.bar(x - width/2, df_clean['Durations(s) Single'], width, label='SINGLE', color='#4c72b0')
    rects4 = ax2.bar(x + width/2, df_clean['Duration(s) Multi'], width, label='MULTI', color='#c44e52')

    ax2.set_xlabel('Instance ID')
    ax2.set_ylabel('Temps (s)')
    ax2.set_title("Temps d'Exécution")
    ax2.set_xticks(x)
    ax2.set_xticklabels(labels, rotation=45, ha='right')
    ax2.legend()
    ax2.grid(axis='y', linestyle='--', alpha=0.7)

    # Ajustement automatique des marges
    plt.tight_layout()

    # 4. Sauvegarde
    output_file = "comparaison_single_multi.pdf"
    plt.savefig(output_file, bbox_inches='tight')
    print(f"Graphique généré avec succès : {output_file}")
    plt.show()

if __name__ == "__main__":
    single_vs_multi()