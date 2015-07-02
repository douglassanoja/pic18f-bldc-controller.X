#include "puissance.h"
#include "test.h"

/**
 * Correspondance entre vitesse et dur�e de phase.
 */
const int const dureeDePhaseParVitesse[VITESSE_MAX + 1] = {
    1250, 496, 307, 221, 172, 140, 117, 101, 88, 77, 69, 61, 55,
    50, 45, 41, 37, 34, 31, 28, 26, 23, 21, 19, 17, 16, 14, 13, 11, 10
};

/**
 * Puissance actuelle.
 */
static unsigned char puissance = 0;

/**
 * Erreur entre la vitesse demand�e et la vitesse actuelle.
 * La valeur -127 indique que l'erreur n'a pas encore �t� calcul�e.
 */
static signed char e0 = -127;

/**
 * Etablit la puissance de d�part.
 * Est utilis�e pour initialiser la puissance � une valeur possible,
 * en particulier suite � un d�marrage ou � une situation de blocage.
 * @param p La puissance de d�part.
 */
void etablitPuissance(unsigned char p) {
    puissance = p;
    e0 = -127;
}

/**
 * Calcule la puissance selon la vitesse demand�e et la dur�e de la derni�re phase.
 * @param dureeDePhase Dur�e de phase actuelle.
 * @param vitesse Vitesse demand�e.
 * @return Puissance � appliquer.
 */
unsigned char calculePuissance(int dureeDePhaseActuelle, unsigned char vitesseDemandee) {
    static char vitesseActuelle = 1;
    signed char e, de;

    // Obtient la vitesse actuelle selon la dur�e de phase actuelle:
    // (On part de la vitesse pr�c�dente, pour �courter le nombre d'iterations)
    while( dureeDePhaseActuelle < dureeDePhaseParVitesse[vitesseActuelle + 1] ) {
        vitesseActuelle ++;
    }
    while( dureeDePhaseActuelle > dureeDePhaseParVitesse[vitesseActuelle - 1] ) {
        vitesseActuelle --;
    }

    // Limite la vitesse:
    if (vitesseActuelle > VITESSE_MAX) {
        vitesseActuelle = VITESSE_MAX;
        puissance = 5;
    }

    // Effectue les calculs de PID:
    else {
        e = vitesseActuelle - vitesseDemandee;

        if (e0 == -127) {
            de = 0;
        } else {
            de = e - e0;
        }
        e0 = e;

        if (vitesseActuelle < vitesseDemandee) {
            if (de <= 0) {
                puissance ++;
            }
        }
        if (vitesseActuelle > vitesseDemandee) {
            if (de >= 0) {
                puissance --;
            }
        }

        // Limite la puissance:
        if (puissance > 40) {
            puissance = 40;
        }
    }

    // Rend la puissance:
    return puissance;
}

#ifdef TEST
/**
 * Tests unitaires pour le calcul de puissance.
 * @return Nombre de tests en erreur.
 */
unsigned char test_puissance() {
    unsigned char ft = 0;
    unsigned char n;
    unsigned char puissance;

    int dureeDePhase, dureeDePhase0;

    // Tests avec r�ponse sans inertie:
    dureeDePhase = 1000;
    etablitPuissance(6);
    for (n = 0; n < 50; n++) {
        puissance = calculePuissance(dureeDePhase, 15);
        dureeDePhase = 200 - 5 * puissance;
    }
    ft += assertMinMaxInt(dureeDePhase, 35, 46, "PID-05A");

    dureeDePhase = 50;
    etablitPuissance(30);
    for (n = 0; n < 20; n++) {
        puissance = calculePuissance(dureeDePhase, 15);
        dureeDePhase = 200 - 5 * puissance;
    }
    ft += assertMinMaxInt(dureeDePhase, 35, 46, "PID-05B");

    dureeDePhase = 1000;
    etablitPuissance(6);
    for (n = 0; n < 30; n++) {
        puissance = calculePuissance(dureeDePhase, 15);
        dureeDePhase = 200 - 6 * puissance;
    }
    ft += assertMinMaxInt(dureeDePhase, 35, 46, "PID-06A");

    dureeDePhase = 10;
    etablitPuissance(20);
    for (n = 0; n < 30; n++) {
        puissance = calculePuissance(dureeDePhase, 15);
        dureeDePhase = 200 - 6 * puissance;
    }
    ft += assertMinMaxInt(dureeDePhase, 35, 46, "PID-06B");


    // Tests avec inertie:
    dureeDePhase0 = 1000;
    dureeDePhase = 1000;
    etablitPuissance(6);
    for (n = 0; n < 50; n++) {
        puissance = calculePuissance(dureeDePhase, 15);
        dureeDePhase = 200 - 6 * puissance;
        dureeDePhase = (3 * dureeDePhase0 + dureeDePhase) / 4;
        dureeDePhase0 = dureeDePhase;
    }
    ft += assertMinMaxInt(dureeDePhase, 35, 46, "PID-06B");

    return ft;
}
#endif