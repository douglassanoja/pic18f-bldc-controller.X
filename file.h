#ifndef __FILE_H
#define __FILE_H

#include "domaine.h"

/**
 * Groupe un �v�nement et sa valeur associ�e.
 */
struct EVENEMENT_ET_VALEUR {
    /** L'�v�nement. */
    enum EVENEMENT evenement;
    /** Sa valeur associ�e. */
    unsigned char valeur;
};

/**
 * Si diff�rent de z�ro, alors la file a d�bord�.
 */
unsigned char file_alerte = 0;

/**
 * Ajoute un �v�nement � la file.
 * @param evenement �v�nement.
 * @param valeur Valeur associ�e.
 */
void enfileEvenement(enum EVENEMENT evenement, unsigned char valeur);

/**
 * R�cup�re un �v�nement de la file.
 * @return L'�v�nement.
 */
struct EVENEMENT_ET_VALEUR *defileEvenement();

#ifdef TEST
/**
 * Tests unitaires pour la file.
 * @return Nombre de tests en erreur.
 */
unsigned char test_file();
#endif

#endif