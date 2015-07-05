/*
 * File:   pic18f-bldc-trp.c
 * Author: jmgonet
 *
 * Created on March 29, 2015, 10:21 AM
 */
#include <htc.h>
#include <stdio.h>
#include <stdlib.h>

#include "test.h"
#include "domaine.h"
#include "file.h"
#include "puissance.h"
#include "moteur.h"

/**
 * Bits de configuration:
 */
#pragma config FOSC = INTIO67  // Oscillateur interne, ports A6 et A7 comme IO.
#pragma config IESO = OFF      // Pas d'embrouilles avec l'osc. au démarrage.
#pragma config FCMEN = OFF     // Pas de monitorage de l'oscillateur.

// Nécessaires pour ICSP / ICD:
#pragma config MCLRE = EXTMCLR // RE3 est actif comme master reset.
#pragma config WDTEN = OFF     // Watchdog inactif (pour ICSP /ICD)
#pragma config LVP = OFF       // Single Supply Enable bits off.

// Configure les sorties des PWM B et C pour ne pas interférer entre eux
#pragma config CCP2MX = PORTC1  // P2A sur PORTC1
#pragma config CCP3MX = PORTC6  // P3A sur PORTC6

enum STATUS {
    /**
     * Le moteur est en arrêt.
     */
    ARRET,

    /**
     * Le moteur est en train de démarrer. Sa vitesse n'est pas suffisante
     * pour mesurer le BEMF.
     */
    DEMARRAGE,

    /**
     * Le moteur est en mouvement. Sa vitesse est suffisante pour mesurer
     * le BEMF.
     */
    EN_MOUVEMENT,

    /**
     * Le moteur est bloqué.
     */
    BLOQUE
};

#define PUISSANCE_DEMARRAGE 40

void machine(struct EVENEMENT_ET_VALEUR *ev, struct CCP *ccp) {
    static enum STATUS status = ARRET;
    static int dureeDePhase = 0;
    static char dureeBlocage = 0;
    static char phasesDepuisBlocage = 0;
    static unsigned char puissance = 0;
    static signed char vitesse = 0;
    static enum DIRECTION direction = AVANT;

    static unsigned char phase0 = 0;
    unsigned char phase;
    unsigned char p;

    switch(status) {
        case ARRET:
            switch(ev->evenement) {
                case TICTAC:
                    ccp->ccpa = 0;
                    ccp->ccpb = 0;
                    ccp->ccpc = 0;
                    break;

                case VITESSE:
                    vitesse = (signed char) ev->valeur;
                    if (vitesse > 5) {
                        if (vitesse > VITESSE_MAX) {
                            vitesse = VITESSE_MAX;
                        }
                        direction = AVANT;
                        puissance = PUISSANCE_DEMARRAGE;
                        phase0 = 0;
                        status = DEMARRAGE;
                    }

                default:
                    break;
            }
            break;

        case DEMARRAGE:
            switch(ev->evenement) {
                case TICTAC:
                    phase = phaseSelonHall(ev->valeur);
                    if (phase != phase0) {
                        phase0 = phase;
                        calculeAmplitudesArret(puissance, direction, phase, ccp);
                    }
                    dureeDePhase ++;
                    break;

                case VITESSE:
                    vitesse = (signed char) ev->valeur;
                    if (vitesse < 5) {
                        status = ARRET;
                    }
                    if (vitesse > VITESSE_MAX) {
                        vitesse = VITESSE_MAX;
                    }
                    break;

                case BLOCAGE:
                    phasesDepuisBlocage = 0;
                    dureeBlocage ++;
                    if (dureeBlocage > 5) {
                        // status = BLOQUE;
                    }
                    break;

                case PHASE:
                    p = phaseSelonHallEtDirection(ev->valeur, AVANT);
                    if (p != ERROR) {
                        phase = p;
                        dureeBlocage = 0;
                        phasesDepuisBlocage ++;
                        dureeDePhase = 0;
                    }
                    break;
            }
            break;

        // Le moteur a atteint une vitesse de croisière, il est
        // possible d'utiliser les détecteurs hall pour détecter les
        // changements de phase.
        case EN_MOUVEMENT:
            // Pas encore implémenté.
            // Passe directement en blocage.

        // Il n'est pas possible de sortir d'un cas de blocage définitif.
        case BLOQUE:
            ccp->ccpa = 0;
            ccp->ccpb = 0;
            ccp->ccpc = 0;
            break;
    }
}
#ifndef TEST

#undef  PILOTAGE_AVEUGLE
#define PILOTAGE_HALL

#ifdef PILOTAGE_AVEUGLE

#define TEMPS_BLOCAGE 1500

/*
 * Valeur des détecteurs hall pour chaque phase.
 */
const unsigned char const hallParPhase[] = {
    0,
    1, 3, 2, 6, 4, 5,
    0
};

/**
 * Routine de traitement d'interruptions.
 * Pilotage du moteur à l'aveugle. Produit des événements de changement
 * de phase à une vitesse proportionnelle à la lecture du potentiomètre.
 * Sert à expérimenter avec les décalage entre tension moyenne appliquée
 * et la vitesse de rotation.
 */
void low_priority interrupt pilotageAveugle() {
    static unsigned char phase = 1;
    static unsigned char vitesse = 0;
    static unsigned char tempsDernierePhase;
    static unsigned int tempsBlocage = TEMPS_BLOCAGE;
    
    static unsigned char hall0 = 0;
    unsigned char hall;

    // Traitement des conversion AD:
    if (PIR1bits.TMR1IF) {
        PIR1bits.TMR1IF = 0;

        if (!ADCON0bits.GODONE) {
            vitesse = ADRESH >> 2;
            vitesse += 30;
            ADCON0bits.GODONE = 1;

            enfileEvenement(VITESSE, 10);
        }
    }

    // Traitement pour le moteur:
    if (PIR1bits.TMR2IF) {
        PIR1bits.TMR2IF = 0;

        // Evenement TICTAC:
        hall = hallParPhase[phase];
        enfileEvenement(TICTAC, hall);

        // Evenement PHASE:
        if (--tempsDernierePhase == 0) {
            tempsDernierePhase = vitesse;
            phase++;
            if (phase > 6) {
                phase = 1;
            }
            hall = hallParPhase[phase];
            enfileEvenement(PHASE, hall);
        };

        // Par mesure de s�curit�, v�rifie que le moteur tourne vraiment
        // et bloque l'alimentation si il reste trop longtemps sur
        // la m�me position.
        hall = PORTA & 7;
        if (hall != hall0) {
            tempsBlocage = TEMPS_BLOCAGE;
            hall0 = hall;
        }
        if (--tempsBlocage == 0) {
            enfileEvenement(BLOCAGE, hall);
            tempsBlocage = TEMPS_BLOCAGE;
        }
    }
}
#endif

#ifdef PILOTAGE_HALL
#define TEMPS_BLOCAGE 3500
/**
 * Routine de traitement d'interruptions de basse priorit�.
 * Pilotage du moteur sur la base des d�tecteurs Hall.
 */
void low_priority interrupt interruptionsBPTest() {
    unsigned char hall, srv_v;
    static unsigned char hall0 = 0, srv_v0 = 0;
    static unsigned char vitesse = 0;
    static unsigned int tempsDernierePhase = TEMPS_BLOCAGE;

    // Traitement des conversion AD:
    if (PIR1bits.TMR1IF) {
        PIR1bits.TMR1IF = 0;

        if (!ADCON0bits.GODONE) {
            vitesse = ADRESH - 128;
            enfileEvenement(VITESSE, vitesse);
            ADCON0bits.GODONE = 1;
        }
    }

    // Traitement pour le moteur:
    if (PIR1bits.TMR2IF) {
        PIR1bits.TMR2IF = 0;

        // Evenement TICTAC:
        hall = PORTA & 7;
        enfileEvenement(TICTAC, hall);

        // Evenement PHASE:
        if (hall != hall0) {
            tempsDernierePhase = TEMPS_BLOCAGE;
            enfileEvenement(PHASE, hall);
            hall0 = hall;
        }

        // Evenement BLOCAGE:
        if (tempsDernierePhase-- == 0) {
            tempsDernierePhase = TEMPS_BLOCAGE;
            enfileEvenement(BLOCAGE, 0);
        }

        // Evenement VITESSE (lecture de l'entr�e radiocommande sur RA4):
        srv_v = PORTA & 16;
        if (srv_v != 0) {
            vitesse ++;
        }
        if (srv_v != srv_v0) {
            if (srv_v != 0) {
                vitesse = 0;
            } else {
                vitesse <<= 1;
                vitesse -= 90;
                enfileEvenement(VITESSE, vitesse);
            }
            srv_v0 = srv_v;
        }
    }
}
#endif

/**
 * Copie les valeurs CCP vers les sorties appropriées:
 * @param ccp
 */
void etablit(struct CCP *ccp) {
    CCPR1L = (ccp->ccpa == X ? 0 : ccp->ccpa);
    PORTBbits.RB2 = (ccp->ccpa == 0 ? 1 : 0);

    CCPR2L = (ccp->ccpb == X ? 0 : ccp->ccpb);
    PORTCbits.RC0 = (ccp->ccpb == 0 ? 1 : 0);

    CCPR3L = (ccp->ccpc == X ? 0 : ccp->ccpc);
    PORTCbits.RC7 = (ccp->ccpc == 0 ? 1 : 0);
}

/**
 * Point d'entrée.
 * Active les PWM 1 à 3, en mode 'Demi pont', pour produire un PWM
 * à 62Khz, avec une précision de 1024 pas.
 */
void main() {
    struct EVENEMENT_ET_VALEUR *ev;

    struct CCP ccp;

    // Configure le micro controleur pour 64MHz:
    OSCCONbits.IRCF = 7;    // Frequence de base: 16 MHz
    OSCTUNEbits.PLLEN = 1;  // Active le PLL.

    // Configure le module A/D:
    ANSELA = 0b00100000; // Active AN4 (RA5) comme entr�e analogique.
    ANSELB = 0x00;       // D�sactive les convertisseurs A/D.
    ANSELC = 0x00;       // D�sactive les convertisseurs A/D.

    ADCON2bits.ADFM = 0; // Resultat justifie sur ADRESH.
    ADCON2bits.ACQT = 5; // Temps d'aquisition: 12 TAD
    ADCON2bits.ADCS = 6; // TAD � 1uS pour Fosc = 64MHz

    ADCON0bits.CHS = 4;  // Canal AN4 (RA5).
    ADCON0bits.ADON = 1; // Active le module A/D.

    // Configure les ports IO:
    TRISA = 0xFF;       // Tous les bits du port A comme entr�es.
    TRISB = 0x00;       // Tous les bits du port B comme sorties.
    TRISC = 0x00;       // Tous les bits du port C comme sorties.

    // Active le temporisateur 1 (pour piloter les conversions A/D):
    T1CONbits.TMR1ON = 1;   // Active le temporisateur 1
    T1CONbits.TMR1CS = 0;   // Source: Fosc / 4
    T1CONbits.T1CKPS = 0;   // Pas de division de fr�quence.
    T1CONbits.T1RD16 = 1;   // Temporisateur de 16 bits.

    // Active le temporisateur 2 (pour g�rer les PWM):
    T2CONbits.TMR2ON = 1;
    T2CONbits.T2CKPS = 1;   // Pas de division de fr�quence.
    T2CONbits.T2OUTPS = 0;  // Pas de division de fr�quence.
    PR2 = 255;              // P�riode max: 64MHz / (4 * 255) = 62kHz.

    // Active les CCP 1 à 3, tous sur le TMR2:
    CCP1CONbits.CCP1M = 12;         // Sorties P1A, P1B actives à niveau haut.
    CCP1CONbits.P1M = 0;           // Contrôleur de demi pont (P1A et P1B).
    PWM1CONbits.P1DC = TEMPS_MORT;  // Temps mort entre sorties complémentaires.
    CCPTMRS0bits.C1TSEL = 0;        // Utilise TMR2.

    CCP2CONbits.CCP2M = 12;         // Sorties P2A et P2B actives à niveau haut.
    CCP2CONbits.P2M1 = 0;           // Contrôleur de demi pont (P2A et P2B).
    PWM2CONbits.P2DC = TEMPS_MORT;  // Temps mort entre sorties complémentaires.
    CCPTMRS0bits.C2TSEL = 0;        // Utilise TMR2.

    CCP3CONbits.CCP3M = 12;         // Sorties P3A et P3B actives à niveau haut.
    CCP3CONbits.P3M1 = 0;           // Contrôleur de demi pont (P3A et P3B).
    PWM3CONbits.P3DC = TEMPS_MORT;  // Temps mort entre sorties complémentaires.
    CCPTMRS0bits.C3TSEL = 0;        // Utilise TMR2.

    // Active les interruptions en général:
    RCONbits.IPEN = 1;
    INTCONbits.GIEH = 1;
    INTCONbits.GIEL = 1;

    // Active les interruptions du temporisateur 1:
    PIE1bits.TMR1IE = 1;    // Active les interruptions.
    IPR1bits.TMR1IP = 0;    // Interruptions de basse priorité.
    
    // Active les interruptions du temporisateur 2:
    PIE1bits.TMR2IE = 1;    // Active les interruptions.
    IPR1bits.TMR2IP = 0;    // Interruptions de basse priorité.

    // Surveille la file d'événements, et les traite au fur
    // et à mesure:
    while(file_alerte == 0) {
        ev = defileEvenement();
        if (ev != 0) {
            machine(ev, &ccp);
            etablit(&ccp);
        }
    }

    // La file a débordé:
    CCPR1L = 0;
    CCPR2L = 0;
    CCPR3L = 0;

    // Tout s'arrête:
    while(1);
}
#endif

#ifdef TEST
/**
 * Point d'entrée pour les tests unitaires.
 */
void main() {
    unsigned char ft = 0;

    // Initialise la EUSART pour pouvoir écrire dans la console
    // Activez la UART1 dans les propriétés du projet.
    EUSART_Initialize();
    printf("Lancement des tests...\r\n");

    // Exécution des tests:
    ft += test_moteur();

    ft += test_puissance();

    ft += test_file();

    // Affiche le résultat des tests:
    printf("%u tests en erreur\r\n",ft);
    SLEEP();
}
#endif