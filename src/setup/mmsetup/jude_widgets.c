#include "jude_widgets.h"
#include "jude.h"
#include "karljr.h"


const char str_lbx_prior[] = "[UP]";
const char str_lbx_next[]  = "[DOWN]";


//LISTBOX

#define RECAST_T(type, source)((type __huge *)(source))


void judeLBxPresent(void) {
	uint32_t data; // data
  uint32_t self; //self
	uint8_t lines = 0;
	uint16_t colour = CLR_TEXT;
	uint16_t clrtmp;
	uint32_t lineptr;
	uint8_t y, x, w, f, f2, n, pgh, pgl, pgm, cl;

	//zptrself = zptrself;
  self = kzp.self;
	karlObjExcludeState(STATE_DIRTY);

	f = RECAST_T(karlObject_t, self)->state & STATE_ENABLED;
	if 	(!f)
		colour = CLR_SHADOW;

	y = RECAST_T(judeElement_t, self)->posy;
	x = RECAST_T(judeElement_t, self)->posx;
	w = RECAST_T(judeElement_t, self)->width;

	cl = (RECAST_T(judeListBox_t, self)->currline == 0xFF) ? 0 : RECAST_T(judeListBox_t, self)->currline;

	data = (uint32_t)(RECAST_T(judeListBox_t, self))->lines_p;

	pgl = (RECAST_T(judeListBox_t, self)->linesoff > 0) ? 1 : 0;
	pgm = (((RECAST_T(judeListBox_t, self)->linescnt - RECAST_T(judeListBox_t, self)->linesoff) + pgl) > RECAST_T(judeElement_t, self)->height) ? 1 : 0;
	pgh = RECAST_T(judeElement_t, self)->height - (pgl + pgm);

	lines = pgh;

	if  (lines > RECAST_T(judeListBox_t, self)->linescnt) {
		lines = RECAST_T(judeListBox_t, self)->linescnt; 
  }
	
	if  ((lines + RECAST_T(judeListBox_t, self)->linesoff + pgl) > RECAST_T(judeListBox_t, self)->linescnt) {
		lines = RECAST_T(judeListBox_t, self)->linescnt - RECAST_T(judeListBox_t, self)->linesoff;
  }

	f2 = (RECAST_T(karlObject_t, self)->state & STATE_PICKED);
	RECAST_T(karlObject_t, self)->tag = 1;

	judeEraseBkg(colour);

  //if (self != zptrself) {
    //*(uint8_t *)0xd020 = 2;
  //}

//POKE(0xD020U, 0);

	if 	(lines) {
//			if  (f2)
//				POKE(0xD020U, 7);
//			else
//				POKE(0xD020U, 5);

    lineptr = data + (RECAST_T(judeListBox_t, self)->linewidth * RECAST_T(judeListBox_t, self)->linesoff);

    n = 0;
    if 	(RECAST_T(judeListBox_t, self)->linesoff) {
      data = (uint32_t)((char __huge *)(str_lbx_prior));

      if  (cl == n)
        clrtmp = (f) ? (RECAST_T(karlObject_t, self)->state & STATE_ACTIVE) ? CLR_MONEY : CLR_PAPER  :  colour;
      else
        clrtmp = colour;

      if  ((RECAST_T(judeListBox_t, self)->hotline == n) && f && f2)
        clrtmp = CLR_FOCUS;

      judeEraseLine(w, x, y, clrtmp);
      
      uint8_t indent = (uint8_t)(w - sizeof(str_lbx_prior) - 1) >> 1;
      judeDrawTextDirect(clrtmp, indent, w, 0x00, x, y, 0, data);
      y++;
      n++;
    }

    //if (self != zptrself) {
      //*(uint8_t *)0xd020 = 2;
    //}
  
    while (lines) {
      if  (RECAST_T(judeListBox_t, self)->selline == (n + RECAST_T(judeListBox_t, self)->linesoff - pgl))
        clrtmp = (f) ? CLR_FOCUS : CLR_PAPER;
      else if  (cl == n)
        clrtmp = (f) ? (RECAST_T(karlObject_t, self)->state & STATE_ACTIVE) ? CLR_MONEY : CLR_PAPER  : colour;
      else
        clrtmp = colour;

      if  ((RECAST_T(judeListBox_t, self)->hotline == n) && f && f2)
        clrtmp = (clrtmp == CLR_FOCUS) ? CLR_PAPER : CLR_FOCUS;

      judeEraseLine(w, x, y, clrtmp);
      judeDrawTextDirect(clrtmp, 0, w, 0x00, x, y, 0, lineptr);
      y++;
      n++;
      
      lineptr+= RECAST_T(judeListBox_t, self)->linewidth /*64*/;	
      lines--;
    }

    //if (self != zptrself) {
      //*(uint8_t *)0xd020 = 2;
    //}
  
    if 	((RECAST_T(judeListBox_t, self)->linescnt - RECAST_T(judeListBox_t, self)->linesoff) > pgh) {
      if  (cl == n)
        clrtmp = (f) ? (RECAST_T(judeListBox_t, self)->_control._element._object.state & STATE_ACTIVE) ? CLR_MONEY : CLR_PAPER : colour;
      else
        clrtmp = colour;

      if  ((RECAST_T(judeListBox_t, self)->hotline == n) && f && f2)
        clrtmp = CLR_FOCUS;

      data = (uint32_t)((char __huge *)(str_lbx_next));

      judeEraseLine(w, x, y, clrtmp);

      uint8_t indent = (uint8_t)(w - sizeof(str_lbx_next) - 1) >> 1;
      judeDrawTextDirect(clrtmp, indent, w, 0x00, x, y, 0, data);

      //while(1) {
        //__asm(" inc 0xd020 ");
      //}

      y++;
      n++;
      lines--;
    }

    //if (self != zptrself) {
      //*(uint8_t *)0xd020 = 2;
    //}
  }
}

void judeLBxChange(void) {
	karlFarPtr_t data;
	judeListBox_t *self;
	uint8_t dwn, flg = 0;
	uint8_t h;
	
  void (*sel)(void);

	self = (judeListBox_t *)(zptrself);

	flg = (self->_control._element._object.state & (STATE_ENABLED | STATE_VISIBLE));

	dwn = (flg && (self->_control._element._object.state & STATE_DOWN));

	if  (flg && (mouseXCol >= self->_control._element.posx) && 
      (mouseXCol < (self->_control._element.posx + self->_control._element.width)) && 
      (mouseYRow >= self->_control._element.posy) &&   
      (mouseYRow < (self->_control._element.posy + self->_control._element.height))) {
//  flg = 1;
		if  (self->_control._element._object.state & STATE_PICKED) {
			self->hotline = mouseYRow - self->_control._element.posy;
			if  (self->hotline >= self->linescnt) 
				self->hotline = self->linescnt - 1;
			if  (self->hotline > (self->linescnt - self->linesoff))
				self->hotline = self->linescnt - self->linesoff;

			self->_control._element._object.tag = 0;

			karlObjIncludeState(STATE_DIRTY);
		}

		if  (self->_control._element._object.state & STATE_DOWN) {
      uint8_t btns = (self->linesoff > self->_control._element.height) ?
          (self->linescnt > self->_control._element.height + 1) ? 2 : 1 : 0;

			h = (self->_control._element.height - btns);

			if  ((self->linesoff > 0) && (self->hotline == 0)) {
				if  ((self->linesoff - h) < 0)
					self->linesoff = 0;
				else 
					self->linesoff-= h;
				flg = 0;
			} else if ((self->hotline == (self->_control._element.height - 1)) && 
                 ((self->linescnt - self->linesoff) > h)) {
				if ((self->linesoff + h) > self->linescnt)
					self->linesoff = self->linescnt - h;
				else
					self->linesoff+= h;

				flg = 0;
			}
		}

		if (!flg) {
			self->hotline = 0x00;
			self->currline = 0x00;
		} else if (dwn) {
			self->currline = self->hotline;
		}
	};
	
	judeDefCtlChange();

	if (dwn && flg && (self->currline != 0xFF)) {
		self->selline = self->linesoff + self->currline - ((self->linesoff > 0) ? 1 : 0);
		sel = (void(*)(void))self->select;
		if (sel)
			sel();
	}
}

void	judeLBxPressed(void) {
//***FIXME.  I need to address these "correctly"
	uint8_t key = zvalkey & 0xff;
	uint8_t mod = (zvalkey & 0xff00) >> 8;
	uint8_t flg = 0;
	uint8_t h, l;
	void (*sel)(void);

	//karlDWFarPtr_t data;
	//data.data = zptrself;
	judeListBox_t *self = (judeListBox_t*)(zptrself);

//	flg = (self->_control._element._object.state &
//			(STATE_ACTIVE| STATE_ENABLED | STATE_VISIBLE));

	flg = 0;

	if  (self->_control._element._object.state & (STATE_ACTIVE)) {
		l = (self->linescnt - self->linesoff) - ((self->linesoff > 0) ? 0 : 1);
		if  (l > (self->_control._element.height - 1))
			l = (self->_control._element.height - 1);

		switch (key) {
			case KEY_C64_CRIGHT:
			case KEY_C64_CRIGHT | 0x80:
			case KEY_M65_TAB:
      case KEY_M65_SHTAB:
  				//judeUnDownCtrl();
					_judeMoveActiveControl();
          break;
			case KEY_C64_CDOWN:
			case KEY_C64_CDOWN | 0x80:
					//if (!(mod & KEY_MOD_SHIFT)) 
          if (key & 0x80) {
            if (self->currline)
						  self->currline--;
          }	else {
            if (self->currline < l)
						  self->currline++;
          }

					self->hotline = self->currline;

					self->_control._element._object.tag = 0;

					karlObjIncludeState(STATE_DIRTY);
//				judeUnDownCtrl();
				break;
			case KEY_ASC_CR:
				h = (self->_control._element.height - 2);

				if  ((self->linesoff > 0) && (self->currline == 0)) {
					if  ((self->linesoff - h) < 0)
						self->linesoff = 0;
					else 
						self->linesoff-= h;
					flg = 1;
				} else if ((self->currline == (self->_control._element.height - 1)) && 
              ((self->linescnt - self->linesoff) > h)) {
					if ((self->linesoff + h) > self->linescnt)
						self->linesoff = self->linescnt - h;
					else
						self->linesoff+= h;

					flg = 1;
				}

				self->hotline = self->currline;

				self->_control._element._object.tag = 0;
				karlObjIncludeState(STATE_DIRTY);

				if (flg) {
					self->currline = 0x00;
				} else {
					self->selline = self->linesoff + self->currline - ((self->linesoff > 0) ? 1 : 0);

					sel = (void(*)(void))self->select;
					if (sel)
 						sel();
				
					judeUnDownCtrl();
				}

				break;	
		}
	
		karl_errorno = ERROR_ABORT;
	} else
		judeDefCtlKeypress();
}
