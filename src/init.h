#ifndef __INIT_H
#define __INIT_H

/**
 * @brief Initialises the gfx module.
 *
 * This function does all initialisation of all sub-modules. It needs to be called
 * at the beginning of the autoboot prg.
 */
void global_init(void);

void diskio_init(void);

#endif // __INIT_H
