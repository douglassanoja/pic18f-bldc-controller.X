#ifndef __DOMAINE_H
#define __DOMAINE_H

// Quelques constantes:
#define ERROR 255
#define CYCLE 36
#define TEMPS_MORT 12
#define CCPR_MIN 0

/**
 * Liste des événements du système.
 */
typedef enum EVENEMENT {
    /** Pas d'événement. Ne pas utiliser.*/
    AUCUN_EVENEMENT,
            
    /** L'intervalle de temps qui sert de base de calcul s'est écoulé.*/
    BASE_DE_TEMPS,
            
    /** Le moteur vient de changer de phase.*/
    MOTEUR_PHASE,

    /** Le moteur est bloqué (il s'est écoulé trop de temps depuis le dernier changement de phase). */
    MOTEUR_BLOCAGE,

    /** La tension moyenne à appliquer au moteur à changé. */
    MOTEUR_TENSION_MOYENNE,
            
            /***/
    MOTEUR_PWM,

    /** Lecture du potentiomètre.*/
    LECTURE_POTENTIOMETRE,
    
    /** Lecture de la tension d'alimentation. */
    LECTURE_ALIMENTATION,
    
    /** Lecture du courant consommé par le moteur. */
    LECTURE_COURANT,
    
    /** Lecture de la température du moteur. */
    LECTURE_TEMPERATURE,
    
    /** Lecture de l'entrée Avant/Arrière de la radiocommande. */
    LECTURE_RC_AVANT_ARRIERE,
    
    /** Lecture de l'entrée Gauche/Droite de la radiocommande. */
    LECTURE_RC_GAUCHE_DROITE,
    
    /** La vitesse demandée pour le moteur a été spécifiée / modifiée. */
    VITESSE_DEMANDEE,

    /** La vitesse actuelle du moteur a été mesurée. */
    VITESSE_MESUREE,
        
            
             
} Evenement;

/** Pour indiquer le signe d'une magnitude absolue.*/
typedef enum DIRECTION {
    /** La direction n'est pas connue. */
    INDETERMINEE,
    /** Marche avant. */
    AVANT,
    /** Marche arrière. */
    ARRIERE,
    /** La magnitude est signée. */
    SIGNEE,
    /** La magnitude est positive. */
    POSITIVE
} Direction;

/**
 * Groupe un événement et sa valeur associée.
 */
typedef struct EVENEMENT_ET_VALEUR {
    /** L'événement. */
    enum EVENEMENT evenement;
    /** Sa valeur associée. */
    unsigned char valeur;
} EvenementEtValeur;

/** 
 * Décrit une valeur en termes de direction et de magnitude. 
 * Sert à spécifier des vitesses ou des puissances.
 */
typedef struct {
    Direction direction;
    unsigned char magnitude;
} MagnitudeEtDirection;

#endif