# PRNG

## Prérequis

- CMake >= 3.20
- GCC >= 13 ou Clang >= 16 (C++23 requis pour `std::format` et `std::popcount`)
- Git (pour que CMake télécharge spdlog automatiquement au premier build)

## Build

```sh
git clone https://github.com/Darleanow/PRNG && cd PRNG
cmake -S . -B build
cmake --build build --parallel
```

Le binaire est disponible dans `build/prng_app`.

## Lancer les PoC

```sh
./build/prng_app --lfsr    -v    # Partie 1 : reconstruction algébrique LFSR
./build/prng_app --mt      -v    # Partie 2 : clonage MT19937
./build/prng_app --session -v    # Partie 3 : usurpation de session
```

Ajouter `-vv` ou `-vvv` pour plus de détails à l'exécution.

## Documentation

La documentation API générée par Doxygen est disponible sur GitHub Pages :
https://darleanow.github.io/PRNG

Pour la générer localement :

```sh
cmake --build build --target docs
# Ouvrir build/docs/html/index.html
```

---

## LFSR - Attaque par reconstruction algébrique sur GF(2)

Un LFSR Fibonacci 32 bits produit un bit par cycle en XORant un sous-ensemble fixe de positions du registre (les *taps*) et en décalant à droite. La suite de sortie obéit à une récurrence linéaire sur GF(2) : chaque bit est le XOR des 32 bits qui le précèdent, pondéré par le masque de taps.

### Pourquoi 2n bits suffisent

Observer 64 bits consécutifs donne 32 équations à 32 inconnues, les coefficients de tap. Chaque équation exprime un bit de sortie comme combinaison linéaire des 32 bits qui le précèdent. On empile tout dans une matrice et on résout par élimination de Gauss-Jordan sur GF(2). Le polynôme étant primitif, le système est toujours de rang plein et la solution est unique.

### Retrouver l'état initial

Une fois les taps connus, les mêmes 64 bits permettent de retrouver la graine. Chaque bit observé s'exprime comme combinaison linéaire des 32 bits inconnus de l'état initial, en déroulant la récurrence symboliquement depuis t=0. Même solveur, second système.

### Prédire les bits futurs

Avec les taps et la graine, on clone le LFSR, on avance jusqu'à la fin de la fenêtre observée et on génère autant de bits futurs que voulu, identiques au flux original. L'attaque est totale : le générateur n'offre aucune sécurité dès qu'une fenêtre de 64 bits de sortie est connue.

---

## MT19937 - Clonage d'état par inversion du tempering

### Structure interne

MT19937 maintient un tableau de 624 mots de 32 bits et un index courant. Quand l'index atteint 624, une opération de *twist* régénère l'intégralité du tableau via une transformation linéaire sur GF(2). Chaque mot passe ensuite par une fonction de *tempering* avant d'être retourné.

### Fonction de tempering

Le tempering applique quatre opérations de bit-mixing sur le mot brut `y` :

```
y ^= y >> 11
y ^= (y << 7)  & 0x9d2c5680
y ^= (y << 15) & 0xefc60000
y ^= y >> 18
```

Ces quatre opérations sont toutes inversibles. C'est là le problème fondamental : l'état interne n'est pas protégé par une fonction à sens unique.

### Inversion du tempering

Chaque opération s'inverse par propagation de bits par tranches :

- **Décalage droit XOR** (`y ^= y >> s`) : les `s` bits de poids fort sont inchangés, chaque tranche suivante se récupère en XORant avec les bits déjà récupérés au-dessus.
- **Décalage gauche XOR-AND** (`y ^= (y << s) & m`) : même raisonnement depuis le bas, les `s` bits de poids faible sont inchangés.

En appliquant les quatre inversions dans l'ordre inverse, on récupère le mot brut de l'état interne.

### L'attaque

Observer 624 sorties consécutives suffit : chaque sortie est exactement le tempering d'un mot d'état. Inverser le tempering sur chacune reconstruit l'état complet après le dernier twist. Le générateur est alors entièrement cloné.

---

## Scénario réel - Usurpation de session

Un serveur génère des tokens de session via MT19937 seedé à `time()`. Un attaquant qui observe 624 tokens consécutifs en tant que client légitime peut reconstruire l'état interne complet et prédire le token de n'importe quel utilisateur suivant avant même qu'il se connecte.

---

## Cas réels de PRNG faibles

**PHP `rand()` / `mt_rand()`** : jusqu'en PHP 7.1, une version modifiée de MT19937 comportait un bug dans la fonction de twist. La même attaque par observation de 624 sorties s'appliquait directement. Depuis PHP 8.0, `mt_rand()` utilise l'implémentation correcte mais reste un PRNG non cryptographique, à ne jamais utiliser pour des tokens de session.

**Java `java.util.Random`** : LCG 48 bits avec `seed = seed * 0x5deece66dL + 0xbL`. Deux appels à `nextInt()` suffisent à retrouver le seed complet, soit par inversion directe de la relation linéaire, soit par recherche exhaustive sur les 2^16 valeurs restantes.

**Générateurs embarqués** : beaucoup de microcontrôleurs exposent un LFSR matériel non masqué comme source d'aléa. Sur certaines puces NXP LPC et STM32 anciennes, le registre était accessible en lecture via JTAG, ce qui rendait toute dérivation de clé triviale.

---

## CSPRNG - Pourquoi ils résistent

Un CSPRNG ajoute deux propriétés que MT19937 et les LFSR n'ont pas.

**Imprévisibilité vers l'avant** : observer n sorties ne permet pas de calculer la sortie n+1. L'état interne transite par une fonction à sens unique avant chaque sortie.

**Résistance au retour en arrière** : même si l'état courant fuite, les sorties passées restent opaques.

**ChaCha20**, utilisé dans `/dev/urandom` sur Linux depuis le noyau 4.8, produit chaque bloc via 20 tours d'un ARX (Add-Rotate-XOR) sur un état 512 bits. Inverser un seul tour revient à casser une permutation conçue pour être indiscernable d'un tirage uniforme.

**/dev/urandom** collecte de l'entropie depuis les interruptions matérielles, les timings disque et réseau, et les événements clavier. L'état interne n'est jamais exposé directement et la sortie est toujours une dérivation via ChaCha20. Observer toutes les sorties possibles ne révèle rien sur l'état futur ni passé.
