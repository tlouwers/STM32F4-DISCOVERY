/**
 * \file Board.hpp
 *
 * \licence "THE BEER-WARE LICENSE" (Revision 42):
 *          <terry.louwers@fourtress.nl> wrote this file. As long as you retain
 *          this notice you can do whatever you want with this stuff. If we
 *          meet some day, and you think this stuff is worth it, you can buy me
 *          a beer in return.
 *                                                                Terry Louwers
 *
 * \brief   Helper class intended to configure the pins and clock of the system.
 *
 * \details Intended use it to have a single grouping of functionality which
 *          sets the clock and pins of the board into a defined state, and later
 *          into a defined sleep state (for low power behavior).
 *
 * \author      T. Louwers <terry.louwers@fourtress.nl>
 * \version     1.0
 * \date        04-2019
 */

#ifndef BOARD_HPP_
#define BOARD_HPP_

/************************************************************************/
/* Class declaration                                                    */
/************************************************************************/
/**
 * \brief   Board configuration class.
 */
class Board
{
public:
    static void InitPins();
    static bool InitClock();
    static void Sleep();
};


#endif  // BOARD_HPP_
