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

    ax2.set_xlabel('Instance')
    ax2.set_ylabel('Temps (s)')
    ax2.set_title("Temps d'Exécution")
    ax2.set_xticks(x)
    ax2.set_xticklabels(labels, rotation=45, ha='right')
    ax2.legend()
    ax2.grid(axis='y', linestyle='--', alpha=0.7)

    # Ajustement automatique des marges
    plt.tight_layout()

    # 4. Sauvegarde
    output_file = "single_vs_multi.pdf"
    plt.savefig(output_file, bbox_inches='tight')
    print(f"Graphique généré avec succès : {output_file}")
    plt.show()

def mip_vs_dp():
    # 1. Lecture du fichier CSV
    file_path = "results/mip_vs_dp.csv" 
    try:
        # Adaptez 'sep' selon votre fichier (souvent ';' ou ',')
        df = pd.read_csv(file_path, sep=";")
    except FileNotFoundError:
        print(f"Erreur: Le fichier '{file_path}' est introuvable.")
        return

    # Nettoyage des noms de colonnes
    df.columns = df.columns.str.strip()

    # 2. Renommage pour faciliter l'accès (MIP vs DP)
    # On cible spécifiquement les colonnes de temps. 
    # Attention aux 's' : "Durations(s)" (MIP) vs "Duration(s)" (DP)
    df = df.rename(columns={
        'Durations(s)': 'Time MIP',
        'Duration(s)': 'Time DP'
    })

    # 3. Tri des données par taille d'instance
    # On extrait le nombre X de "uniform_X" pour trier numériquement (2, 3, ..., 10, ...)
    df['Size'] = df['Instance'].apply(lambda x: int(str(x).split('_')[-1]) if '_' in str(x) else 0)
    df = df.sort_values('Size')

    # Préparation de l'axe X
    labels = df['Size'].astype(str)
    x = np.arange(len(labels))
    width = 0.35  # Largeur des barres

    # 4. Création du Graphique Unique
    fig, ax = plt.subplots(figsize=(12, 6))

    # Barres pour le MIP (Bleu)
    rects1 = ax.bar(x - width/2, df['Time MIP'], width, label='MIP Pricing', color='#4c72b0')
    
    # Barres pour le DP (Rouge)
    rects2 = ax.bar(x + width/2, df['Time DP'], width, label='Pricing Dynamique', color='#c44e52')

    # Mise en forme
    ax.set_xlabel('Instance', fontsize=12)
    ax.set_ylabel('Temps d\'exécution (secondes)', fontsize=12)
    ax.set_title('MIP vs Programmation Dynamique', fontsize=14)
    
    ax.set_xticks(x)
    ax.set_xticklabels(labels, rotation=90, fontsize=9) # Rotation pour lire toutes les instances
    ax.legend()
    
    # Grille horizontale pour la lisibilité
    ax.grid(axis='y', linestyle='--', alpha=0.7)

    # Ajustement des marges
    plt.tight_layout()

    # 5. Sauvegarde
    output_file = "mip_vs_dp.pdf"
    plt.savefig(output_file, bbox_inches='tight')
    print(f"Graphique généré : {output_file}")
    plt.show()

def without_vs_with_inout():
    # 1. Lecture du fichier CSV
    file_path = "results/without_vs_with_inout.csv"
    
    try:
        df = pd.read_csv(file_path, sep=";")
    except FileNotFoundError:
        print(f"Erreur: Le fichier '{file_path}' est introuvable.")
        return

    # Nettoyage des espaces dans les noms de colonnes (ex: " Durations(s)" -> "Durations(s)")
    df.columns = df.columns.str.strip()

    # 2. Renommage des colonnes pour gérer les doublons et clarifier
    # Pandas renomme automatiquement le 2ème "Nb cols" en "Nb cols.1"
    # On renomme tout pour être sûr de qui est qui.
    # Structure attendue: 
    # [Instance, NOSTAB Value, Nb cols, Durations(s), INOUT Value, Nb cols.1, Duration(s)]
    
    rename_map = {
        'Nb cols': 'Nb cols NOSTAB',
        'Nb cols.1': 'Nb cols INOUT',   # La 2ème occurrence (colonnes 6)
        'Durations(s)': 'Time NOSTAB',  # Notez le pluriel dans votre header
        'Duration(s)': 'Time INOUT'     # Notez le singulier dans votre header
    }
    df = df.rename(columns=rename_map)

    # 3. Filtrage des données (Gestion des TLR - Time Limit Reached)
    # On filtre sur la colonne NOSTAB Value
    mask_solved = ~df['NOSTAB Value'].astype(str).str.contains(r'\(TLR\)')
    df_clean = df[mask_solved].copy()

    if df_clean.empty:
        print("Attention: Aucune instance résolue (toutes sont TLR).")
        return

    # Extraction des labels (ex: "uniform_12" -> 12) pour le tri
    df_clean['Size'] = df_clean['Instance'].apply(lambda x: int(str(x).split('_')[-1]) if '_' in str(x) else 0)
    df_clean = df_clean.sort_values('Size')

    labels = df_clean['Size'].astype(str)
    x = np.arange(len(labels))
    width = 0.35

    # 4. Création des Graphes
    fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(14, 6))

    # --- Graphe 1 : Nombre de Colonnes (NOSTAB vs INOUT) ---
    ax1.bar(x - width/2, df_clean['Nb cols NOSTAB'], width, label='NOSTAB', color='#4c72b0')
    ax1.bar(x + width/2, df_clean['Nb cols INOUT'], width, label='INOUT', color='#c44e52')

    ax1.set_xlabel('Instance Size')
    ax1.set_ylabel('Nombre de Colonnes (Log Scale)')
    ax1.set_title('Colonnes Générées')
    ax1.set_xticks(x)
    ax1.set_xticklabels(labels, rotation=90, fontsize=9)
    ax1.legend()
    ax1.set_yscale('log') # Échelle logarithmique
    ax1.grid(axis='y', linestyle='--', alpha=0.7)

    # --- Graphe 2 : Temps d'exécution ---
    ax2.bar(x - width/2, df_clean['Time NOSTAB'], width, label='NOSTAB', color='#4c72b0')
    ax2.bar(x + width/2, df_clean['Time INOUT'], width, label='INOUT', color='#c44e52')

    ax2.set_xlabel('Instance')
    ax2.set_ylabel('Temps (s)')
    ax2.set_title("Temps d'Exécution")
    ax2.set_xticks(x)
    ax2.set_xticklabels(labels, rotation=90, fontsize=9)
    ax2.legend()
    ax2.grid(axis='y', linestyle='--', alpha=0.7)

    # Ajustement automatique des marges
    plt.tight_layout()

    # 5. Sauvegarde
    output_file = "without_vs_with_inout.pdf"
    plt.savefig(output_file, bbox_inches='tight')
    print(f"Graphique généré avec succès : {output_file}")
    plt.show()
if __name__ == "__main__":
    #single_vs_multi()
    #mip_vs_dp()
    without_vs_with_inout()