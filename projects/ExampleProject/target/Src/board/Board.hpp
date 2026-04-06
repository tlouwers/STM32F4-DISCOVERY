/**
 * \file    Board.hpp
 *
 * \licence "THE BEER-WARE LICENSE" (Revision 42):
 *          <terry.louwers@fourtress.nl> wrote this file. As long as you retain
 *          this notice you can do whatever you want with this stuff. If we
 *          meet some day, and you think this stuff is worth it, you can buy me
 *          a beer in return.
 *                                                                Terry Louwers
 * \class   Board
 *
 * \brief   Helper class intended to configure the pins and clock of the system.
 *
 * \author  T. Louwers <terry.louwers@fourtress.nl>
 * \version 1.0
 * \date    04-2019
 */

#ifndef BOARD_HPP_
#define BOARD_HPP_

/************************************************************************/
/* Class declaration                                                    */
/************************************************************************/
class Board
{
public:
    static void InitPins();
    static bool InitClock();
    static void Sleep();
};


#endif  // BOARD_HPP_
