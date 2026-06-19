# PRNG

## LFSR - Attaque par reconstruction algébrique sur GF(2)

Un LFSR Fibonacci 32 bits produit un bit par cycle en XORant un sous-ensemble fixe de positions du registre (les *taps*) et en décalant à droite. La suite de sortie satisfait une récurrence linéaire sur GF(2) : chaque bit est un XOR des 32 bits précédents, pondéré par le masque de taps.

### Pourquoi 2n bits suffisent

Observer 64 bits consécutifs donne 32 équations à 32 inconnues (les coefficients de tap). Chaque équation exprime un bit de sortie comme combinaison linéaire des 32 bits qui le précèdent. On empile tout dans une matrice et on résout par élimination de Gauss-Jordan sur GF(2). Comme le polynôme est primitif, le système est toujours de rang plein - solution unique garantie.

### Retrouver l'état initial

Une fois les taps connus, les mêmes 64 bits permettent de retrouver la graine. Chaque bit observé s'exprime comme combinaison linéaire des 32 bits inconnus de l'état initial, en déroulant la récurrence symboliquement depuis t=0. Même solveur, second système.

### Prédire les bits futurs

Avec les taps et la graine, on clone le LFSR, on avance jusqu'à la fin de la fenêtre observée, et on génère autant de bits futurs que voulu - identiques au flux original. L'attaque est totale : le générateur n'offre aucune sécurité dès qu'une fenêtre de 64 bits de sortie est connue.

### Lancer le PoC

```sh
cmake -S . -B build && cmake --build build
./build/prng_app --lfsr -v       # attaque complète, vérifie les 3 étapes
./build/prng_app --lfsr -vv      # avec sortie debug
./build/prng_app --lfsr -vvv     # avec trace par équation
```

---

## MT19937 - Clonage d'état par inversion du tempering

### Structure interne

MT19937 maintient un tableau de 624 mots de 32 bits et un index courant. Quand l'index atteint 624, une opération de *twist* régénère l'intégralité du tableau en appliquant une transformation linéaire sur GF(2) basée sur une matrice de recurrence twist. Chaque mot du tableau passe ensuite par une fonction de *tempering* avant d'être retourné à l'appelant.

### Fonction de tempering

Le tempering applique quatre opérations bit-mixing successives sur le mot brut `y` :

```
y ^= y >> 11
y ^= (y << 7)  & 0x9d2c5680
y ^= (y << 15) & 0xefc60000
y ^= y >> 18
```

Ces opérations sont toutes inversibles, ce qui est la faille centrale : l'état interne n'est pas protégé par une fonction à sens unique.

### Inversion du tempering

Chaque opération s'inverse par propagation de bits par tranches :

- **Décalage droit XOR** (`y ^= y >> s`) : les `s` bits de poids fort sont inchangés, chaque tranche suivante se récupère en XORant avec les bits déjà récupérés au-dessus.
- **Décalage gauche XOR-AND** (`y ^= (y << s) & m`) : même raisonnement en partant du bas, les `s` bits de poids faible sont inchangés.

En appliquant les quatre inversions dans l'ordre inverse du tempering, on récupère le mot brut de l'état interne.

### L'attaque

Observer 624 sorties consécutives suffit : chaque sortie est exactement le tempering d'un des 624 mots d'état. Inverser le tempering sur chacune reconstruit l'état complet après le dernier twist. Le générateur est alors entièrement cloné et prédit tous les bits futurs sans erreur.

### Lancer le PoC

```sh
./build/prng_app --mt -v     # clonage complet, 100 prédictions vérifiées
./build/prng_app --mt -vv    # avec détail des observations et prédictions
```
