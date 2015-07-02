#ifndef __MOTEUR_H
#define __MOTEUR_H

#include "domaine.h"

/**
 * Contient les valeurs des rapports cycliques des trois
 * modulations PWM.
 */
struct CCP {
    /** Rapport cyclique pour le connecteur de la bobine A. */
    unsigned char ccpa;
    /** Rapport cyclique pour le connecteur de la bobine B. */
    unsigned char ccpb;
    /** Rapport cyclique pour le connecteur de la bobine C. */
    unsigned char ccpc;
};


/**
 * Rend les valeurs PWM para rapport � l'angle sp�cifi�.
 * � appeler lorsque l'angle est connu, c'est � dire, lorsque le moteur
 * est en mouvement.
 * @param alpha Angle, entre 0 et 35.
 * @param puissance, entre 0 et 64.
 * @param ccp Structure pour les valeurs PWM.
 */
void calculeAmplitudesEnMouvement(unsigned char alpha, unsigned char puissance, struct CCP *ccp);

/**
 * Rend les valeurs PWM para rapport � la phase sp�cifi�e.
 * � appeler lorsque seule la phase est connue, mais pas l'angle, c'est � dire
 * lorsque le moteur est � l'arret.
 * - Calcule l'angle moyen correspondant � la phase sp�cifi�e.
 * - Utilise 'calculeAmplitudesEnMouvement' pour obtenir les valeurs de
 *   PWM correspondantes � cet angle, avec une puissance moyenne.
 * @param phase Phase, entre 1 et 6.
 * @param ccp Structure pour les valeurs PWM.
 * @param puissance Puissance � utiliser. Il est conseill� de ne pas utiliser
 * une valeur trop forte ici, pour ne pas br�ler le circuit.
 */
void calculeAmplitudesArret(unsigned char phase, struct CCP *ccp, unsigned char puissance);

/**
 * Determine la phase en cours d'apr�s les senseurs hall.
 * @param hall La valeur des senseurs hall: 0xb*****yzx
 * @return Le num�ro de phase, entre 1 et 6.
 */
unsigned char phaseSelonHall(unsigned char hall);

/**
 * Calcule la phase en cours � partir de la lecture des senseurs hall.
 * Effectue �galement un contr�le de la lecture, pour v�rifier si elle est
 * possible. Ceci sert � �viter de compter des rebondissements ou du bruit
 * qui affecte la lecture des senseurs.
 * @param hall La valeur des senseurs hall: 0xb*****yzx
 * @param direction Direction actuelle.
 * @return La phase (de 0 � 5) ou un code d'erreur.
 */
unsigned char phaseSelonHallEtDirection(unsigned char hall, enum DIRECTION direction);

/**
 * Calcule l'angle correspondant � la phase et � la direction actuelle
 * de rotation.
 * @param phase Phase actuelle.
 * @param direction Direction actuelle.
 * @return L'angle correspondant.
 */
unsigned char angleSelonPhaseEtDirection(unsigned char phase, enum DIRECTION direction);

/**
 * Cette fonction est appel�e en r�ponse � un changement de phase. �
 * cet instant on connait la valeur exacte des deux param�tres.
 * @param angle Angle exact.
 * @param dureeDePhase Dur�e de la derni�re phase.
 */
void corrigeAngleEtVitesse(unsigned char angle, int dureeDePhase);

/**
 * Cette fonction est appel�e � chaque cycle de PWM pour calculer (estimer)
 * l'angle actuel.
 * Le calcul se fait sur la base du dernier angle connu avec pr�cision et
 * de la dur�e de la derni�re phase. Ces valeurs ont �t� �tablies par l'appel
 * � 'corrigeAngleEtVitesse'.
 * @return L'angle actuel estim�.
 */
unsigned char calculeAngle();

#ifdef TEST
/**
 * Point d'entr�e pour les tests du moteur.
 * @return Nombre de tests en erreur.
 */
unsigned char test_moteur();
#endif

#endif