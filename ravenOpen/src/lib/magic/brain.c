/*
**++
**  RCSID:     $Id: brain.c,v 1.3 2001/03/13 02:16:57 raven Exp $
**
**  FACILITY:  RavenMUD
**
**  LEGAL MUMBO JUMBO:
**
**      This is based on code developed for DIKU and Circle MUDs.
**
**  MODULE DESCRIPTION:
**
**  AUTHORS:
**
**      Mortius from RavenMUD
**
**  NOTES:
**
**      Use 132 column editing in here.
**
**--
*/


/*
**
**  MODIFICATION HISTORY:
**
**  $Log: brain.c,v $
**  Revision 1.3  2001/03/13 02:16:57  raven
**  end_fight() uses
**
**  Revision 1.2  2000/10/10 13:47:04  raven
**
**  Transitioned over to the new include structures.
**
**  Revision 1.1.1.1  2000/10/10 04:15:17  raven
**  RavenMUD 2.0
**
**  Revision 1.6  1998/01/29 03:38:48  digger
**  Removed all references to BLOOD_ENABLED
**
**  Revision 1.5  1997/09/26 05:32:30  vex
**  Code clean up.
**
**  Revision 1.4  1997/09/18 11:00:44  vex
**  Replaced all obj_data, room_data, mob_special_data, char_data,
**  descriptor_data structs with appropriate typedef.
**
**  Revision 1.3  1997/01/03 12:32:45  digger
**  Renamed several of the functions from skills.c and added skill
**  avoidance to fist and hamstring. Vex has put in MAJOR changes
**  to the summoning code and many checks for Book of Blood signatures
**  were added.
**
**
*/


/*
** STANDARD U*IX INCLUDES
*/
#include "general/conf.h"
#include "general/sysdep.h"

#define THIS_SKILL                SKILL_BRAIN

#define SKILL_ADVANCES_WITH_USE   FALSE
#define SKILL_ADVANCE_STRING      "You've become one with the sword."

/*
** SKILL_MAX_LEARN 
**         Define as either a hard number or use the skill/class array
**         scheme to allow certain classes to learn particular skills
**         better than others.
**
** define SKILL_MAX_LEARN         90
**
*/

/*                                Mu  Cl  Th  Wa  Ra  As  Sl  Kn  Dk  Sd  Ba  XX  XX  XX  XX  XX  XX  XX  XX  XX */
static int max_skill_lvls[] =   { 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 90, 00, 00, 00, 00, 00, 00, 00, 00, 00 };
#define SKILL_MAX_LEARN           max_skill_lvls[ (int)GET_CLASS(ch) ]

/*
** If dex, int, or wis affects the speed at which this skill is learned then
** set the following macros to TRUE. This means the user will learn the skill
** a little faster based on those attributes.
*/
#define DEX_AFFECTS               TRUE
#define INT_AFFECTS               FALSE
#define WIS_AFFECTS               FALSE

/*
** These are the two macros that define the range of the stun duration for
** an executed skill. These values are used for both the char and the victim.
*/
#define STUN_MIN                  2
#define STUN_MAX                  4

/*
** MUD SPECIFIC INCLUDES
*/
#include <stdlib.h>

#include "general/db.h"
#include "general/structs.h"
#include "general/class.h"
#include "general/comm.h"
#include "general/handler.h"
#include "actions/interpreter.h"
#include "magic/skills.h"
#include "magic/spells.h"
#include "util/utils.h"
#include "actions/fight.h"

/*
**++
**  FUNCTIONAL DESCRIPTION:
**
**      SKILL_NAME - [ Brain ]
**
**  FORMAL PARAMETERS:
**
**      ch:
**          A pointer to the character structure that is trying to use the skill/spell.
**
**  RETURN VALUE:
**
**      None
**
**  DESIGN:
**
**      With this skill the player will take his weapon and smash it off the victims
**	head.  There is a 7% chance of KO'ing the player.  Major damage skill.
**	
**	
**
**  NOTES:
**      The following standard macros can be used from skills.h:
**
**          IF_CH_CANT_SEE_VICTIM( string );    IF_VICTIM_NOT_WIELDING( string );
**          IF_CH_CANT_BE_VICTIM( string );     IF_VICTIM_CANT_BE_FIGHTING( string );
**          IF_CH_NOT_WIELDING( string );       IF_VICTIM_NOT_WIELDING( string );
**          IF_ROOM_IS_PEACEFUL( string );      IF_VICTIM_NOT_STANDING( string );
**          IF_UNLEARNED_SKILL( string );
**
**      The parameter `string' is the string that is sent to the user when the
**      error condition is met and the skill function exits. For example when an
**      attempt to disarm a weaponless victim is made:
**
**          IF_VICTIM_NOT_WIELDING( "That person isn't even wielding a weapon!\r\n" );
**
*/
ACMD(do_brain)
{
    char   arg[100];
    CharData *victim;
    struct affected_type af;
    int dam = number(GET_LEVEL(ch) / 2, GET_LEVEL(ch) * 4);

    one_argument( argument, arg );

    IF_UNLEARNED_SKILL   ( "You wouldn't know where to begin.\r\n" );
    IF_CH_CANT_SEE_VICTIM( "Brain who?\r\n" );
    IF_CH_CANT_BE_VICTIM ( "Aren't we funny today...\r\n" );
    IF_CH_NOT_WIELDING   ( "You need to wield a weapon to make it a success.\r\n" );
    IF_ROOM_IS_PEACEFUL  ( "Your violence has been suppressed.\r\n" );
    IF_VICTIM_NOT_STANDING( "Your victim is not standing.\n\r" );

    if (skillSuccess(ch, THIS_SKILL)) {
        advanceSkill(ch, THIS_SKILL, SKILL_MAX_LEARN, SKILL_ADVANCE_STRING, 
		         DEX_AFFECTS, INT_AFFECTS, WIS_AFFECTS);
        if (number(1, 100) < 7) { /* KO */
            end_fight(victim);

            act("You are knocked out when $n hits you upside your head.",
		 FALSE, ch, 0, victim, TO_VICT);
            act("$N sees stars and slumps over knocked out.",
		 FALSE, ch, 0, victim, TO_CHAR); 
	    act("$N sees stars and slumps over knocked out after $n brains $M.",
		 FALSE, ch, 0, victim, TO_ROOM);
	    GET_POS(victim) = POS_SLEEPING;
	    update_pos(victim);

            af.type      = SPELL_SLEEP;
            af.modifier  = 0;
            af.duration  = 4 TICKS;
            af.bitvector = AFF_SLEEP;
            af.location  = APPLY_NONE;
            affect_join(victim, &af, FALSE, FALSE, FALSE, FALSE);
            STUN_USER_MIN;
            STUN_VICTIM_MAX;
        } else {
             damage(ch, victim, dam, THIS_SKILL);
             STUN_USER_MAX;
        }        
    } else {
         damage(ch, victim, 0, THIS_SKILL);
	STUN_USER_MAX;
    }
}
