# panneau_01
Cet objet permet de simuler un panneau lumineux d'aspect contemporain ou SF. Chaque case du panneau est commandable individuellement via un webUI accessible via le wifi. Les couleurs sont paramétrables

Le contrôle des leds de l’objet se fait entièrement via le webUI de l’objet. Pour y accéder, voir la partie 5.1 “Accès au webUI”

Sur le webUI, il est possible de contrôler des groupes de leds et de les allumer/éteindre dans une couleur choisie


Voici deux exemples de casing plus avancé
le premier est tout simple, il utilise l’anneau de leds du kit technoLARP
le deuxième est plus abouti, il utilise 16 leds sur un ruban et un boitier en bois mdf peint

Dans les 2 cas, les leds se trouvent derrière un diffuseur de lumière (une matière blanche translucide) et d’un masque (matière opaque) qui permet de former un texte visible à l’oeil nu




**[Exemple](#Exemple)**  
**[Composants](#Composants)**  
**[Branchements](#Branchements)**  
**[BackOffice](#BackOffice)**  
**[Paramètres de gameplay](#param%C3%A8tres-de-gameplay)**  
**[Paramètres Réseau](#param%C3%A8tres-r%C3%A9seau)**  

## Exemple

<img width="1162" height="843" alt="unnamed (7)" src="https://github.com/user-attachments/assets/2cbd8198-fb73-45c4-b213-79d54300e6c1" />


## Composants
Vous aurez besoin pour monter le panneau 01 :

|  | |
| :---------------- | :------: |
| Un kit technoLarp | <img src="./images/technolarp_pcb_wemos.jpg" width="200"> |
| Un câble micro-USB | <img src="./images/technolarp_cable_micro-usb.png" width="200"> |
| Un ruban ou d’un anneau de led ws2812b ou neopixel | <img src="./images/technolarp_leds_ws2812b_01.jpg" height="200"> |
| Une batterie 18650 et son support | <img src="./images/technolarp_18650.jpg" width="200"> |

## Branchements
Connecter les composants sur le kit technoLARP comme sur cette photo :
<img width="1215" height="935" alt="unnamed (6)" src="https://github.com/user-attachments/assets/d2c06e2f-6592-4b83-9332-414aca463ac5" />




## Installation
Pour installer le firmware de l'objet, il faut suivre ce [tutorial](https://github.com/technolarp/technolarp.github.io/wiki/Installation-du-firmware)  


## BackOffice
Pour se connecter au back office de l'objet, il faut suivre ce [tutorial](https://github.com/technolarp/technolarp.github.io/wiki/Connexion-au-back-office-de-l'objet-via-le-wifi)  

<img width="704" height="584" alt="unnamed (8)" src="https://github.com/user-attachments/assets/9a45cc97-aaa1-4258-964e-6d303973e675" />



## Paramètres de gameplay

Ces paramètres permettent de contrôler le gameplay de l’objet.



| Nom         | Descriptif                                                                                                                         |
|---------------------|----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| Object name           | Le nom de l’objet, composé de 1 à 20 lettres et chiffres                                                                                                                                                       |
| Object ID             | Un numéro d’identification de l’objet                                                                                                                                                                          |
| Group ID              | Un numéro d’identification du groupe de l’objet                                                                                                                                                                |
| nbColonnes            | Le nombre de colonnes de led sur le panneau. Entre 1 et 5                                                                                                                                                      |
| nbSegments            | Le nombre de segments du panneau. Un segment est un groupe de 1 à 5 leds. Nombre entre 1 et 20.                                                                                                                |
| ledParSegment         | Le nombre de leds groupées pour chaque segment du panneau. Entre 1 et 5                                                                                                                                        |
| ActiveLeds            | Le nombre total de leds utilisées par le panneau. Ce chiffre est calculé automatiquement (= nbColonnes X nbSegments). Il ne doit pas dépasser 25                                                               |
| Brightness            | La luminosité des leds, entre 0 (éteinte) et 255 (pleine intensité)                                                                                                                                            |
| Scintillement         | Le scintillement permet d’activer le scintillement des leds pour un effet visuel plus ou moins rapide. le slider permet de régler la vitesse du scintillement                                                  |
| Nombre couleurs       | Permet de choisir le nombre de couleurs utilisées pour chaque segment.                                                                                                                                         |
| Couleur 1 à couleur 5 | Choix de la teinte de chaque couleur                                                                                                                                                                           |
| LABEL 01 à LABEL 04   | Le nom de chaque segment. Ils peuvent être modifié pour un texte composé de 1 à 20 lettres et chiffres  !! Si cette partie n'apparaît pas, penser à rafraichir la page dans le navigateur ou appuyer sur F5 !! |
| Cercle de couleur     | Les cercles sont des boutons cliquables qui commandent la couleur de chaque segment.                                                                                                                           |
| statut Panneau        | le statut actuel de l’objet. Il peut être  ACTIF  BLINK Les 2 boutons permettent de forcer l’état du panneau                                                                                                   |
## Paramètres Réseau

Pour avoir le descriptif des paramètres réseau suivez ce [lien](https://github.com/technolarp/technolarp.github.io/wiki/Param%C3%A8tres-R%C3%A9seau)  
