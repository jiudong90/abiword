/* AbiWord
 * Copyright (C) 1998,1999 AbiSource, Inc.
 * Copyright (C) 2003-2004 Hubert Figuiere
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.
 */


// TODO: still getting some artifacts when doing highligh/replacements

#include <stdlib.h>
#include <string.h>

#include "ut_string.h"
#include "ut_string_class.h"
#include "ut_assert.h"
#include "ut_debugmsg.h"

#include "xap_CocoaDialog_Utilities.h"

#include "xap_App.h"
#include "xap_CocoaApp.h"
#include "xap_CocoaFrame.h"

#include "ap_Strings.h"
#include "ap_Dialog_Id.h"
#include "ap_Dialog_Spell.h"
#include "ap_CocoaDialog_Spell.h"


XAP_Dialog * AP_CocoaDialog_Spell::static_constructor(XAP_DialogFactory * pFactory, XAP_Dialog_Id dlgid)
{
   AP_CocoaDialog_Spell * p = new AP_CocoaDialog_Spell(pFactory, dlgid);
   return p;
}

AP_CocoaDialog_Spell::AP_CocoaDialog_Spell(XAP_DialogFactory * pDlgFactory, XAP_Dialog_Id dlgid)
  : AP_Dialog_Spell(pDlgFactory, dlgid),
	m_dlg(nil),
	m_suggestionList(nil)
{
}

AP_CocoaDialog_Spell::~AP_CocoaDialog_Spell(void)
{
	[m_suggestionList release];
}

/************************************************************/
void AP_CocoaDialog_Spell::runModal(XAP_Frame * pFrame)
{
	UT_DEBUGMSG(("beginning spelling check...\n"));
	
	// class the base class method to initialize some basic xp stuff
	AP_Dialog_Spell::runModal(pFrame);
	
	m_bCancelled = false;
	bool bRes = nextMisspelledWord();
	
	if (bRes) { // we need to prepare the dialog
		m_dlg = [[AP_CocoaDialog_Spell_Controller alloc] initFromNib];
		
		// used similarly to convert between text and numeric arguments
		[m_dlg setXAPOwner:this];
	
		// build the dialog
		NSWindow * window = [m_dlg window];
		UT_ASSERT(window);

		// Populate the window's data items
		_populateWindowData();
				
		// now loop while there are still misspelled words
		while (bRes) {
			
			// show word in main window
			makeWordVisible();
			
			// update dialog with new misspelled word info/suggestions
			_showMisspelledWord();
			
			// run into the GTK event loop for this window
			[NSApp runModalForWindow:window];
			_purgeSuggestions();
	 
			if (m_bCancelled) {
				break;
			}
	 
			// get the next unknown word
			bRes = nextMisspelledWord();
		}
		
		_storeWindowData();
		
		[m_dlg close];
		[m_dlg release];
		m_dlg = nil;
		[m_suggestionList release];
		m_suggestionList = nil;
	}
   
	// TODO: all done message?
	UT_DEBUGMSG(("spelling check complete.\n"));
}


void AP_CocoaDialog_Spell::_showMisspelledWord(void)
{                                
	NSMutableAttributedString *attrStr;
	NSAttributedString *buffer;
	const UT_UCSChar *p;
	// insert start of sentence
	UT_sint32 iLength;
	
	attrStr = [[NSMutableAttributedString alloc] initWithString:@""];
	[m_dlg setMisspelled:nil];

	p = m_pWordIterator->getPreWord(iLength);
	if (0 < iLength) {
		UT_UTF8String str(p);
		buffer = [[[NSAttributedString alloc] initWithString:[NSString stringWithUTF8String:str.utf8_str()]] autorelease];
		[attrStr appendAttributedString:buffer];
	}

	// insert misspelled word (in highlight color)
	p = m_pWordIterator->getCurrentWord(iLength);
	{
		UT_UTF8String str(p);
		buffer = [[[NSAttributedString alloc] initWithString:[NSString stringWithUTF8String:str.utf8_str()]] autorelease];
	}
	[attrStr appendAttributedString:buffer];
		
	// insert end of sentence
	p = m_pWordIterator->getPostWord(iLength);
	if (0 < iLength) {
		UT_UTF8String str(p);
		[attrStr appendAttributedString:
		            [[[NSAttributedString alloc] initWithString:
					         [NSString stringWithUTF8String:str.utf8_str()]] autorelease]];
	}

	[m_dlg setMisspelled:attrStr];

	[m_suggestionList removeAllStrings];
	for (UT_uint32 i = 0; i < m_Suggestions->getItemCount(); i++) {
		UT_UTF8String str((UT_UCSChar*)m_Suggestions->getNthItem(i));
		[m_suggestionList addUT_UTF8String:str];
	}
	
	if (!m_Suggestions->getItemCount()) {	
		const XAP_StringSet * pSS = m_pApp->getStringSet();
		[m_suggestionList addUT_UTF8String:pSS->getValue(AP_STRING_ID_DLG_Spell_NoSuggestions)];
	
		m_iSelectedRow = -1;
		[m_dlg selectSuggestion:-1];
		
	} else {
		[m_dlg selectSuggestion:0];
	}

	[attrStr release];
}

void AP_CocoaDialog_Spell::_populateWindowData(void)
{
   // TODO: initialize list of user dictionaries
   m_suggestionList = [[XAP_StringListDataSource alloc] init];
   [m_dlg setSuggestionList:m_suggestionList];
}

void AP_CocoaDialog_Spell::_storeWindowData(void)
{
   // TODO: anything to store?
}

/*************************************************************/

void AP_CocoaDialog_Spell::event_Change()
{
	UT_UCSChar * replace = NULL;
	UT_DEBUGMSG(("m_iSelectedRow is %i\n", m_iSelectedRow));
	if (m_iSelectedRow != -1) {
		replace = (UT_UCSChar*) m_Suggestions->getNthItem(m_iSelectedRow);
		UT_DEBUGMSG(("Replacing with %s\n", (char*) replace));
		//fprintf(stderr, "Replacing with %s\n", replace);
		changeWordWith(replace);		
	}
	else {
		UT_UCS4String str([[m_dlg replace] UTF8String], 0);
		changeWordWith(str.ucs4_str());
	}
	
   [NSApp stopModal];
}

void AP_CocoaDialog_Spell::event_ChangeAll()
{
	UT_UCSChar * replace = NULL;
	if (m_iSelectedRow != -1) {
		replace = (UT_UCSChar*) m_Suggestions->getNthItem(m_iSelectedRow);
		addChangeAll(replace);
		changeWordWith(replace);
	}
	else {
		NSString * replace = [m_dlg replace];
		
		UT_UCS4String str([replace UTF8String], 0);

		addChangeAll(str.ucs4_str());
		changeWordWith(str.ucs4_str());
	}
   
   [NSApp stopModal];
}

void AP_CocoaDialog_Spell::event_Ignore()
{
   ignoreWord();
   [NSApp stopModal];
}

void AP_CocoaDialog_Spell::event_IgnoreAll()
{
   addIgnoreAll();
   ignoreWord();
   [NSApp stopModal];
}

void AP_CocoaDialog_Spell::event_AddToDict()
{
   addToDict();
   
   ignoreWord();
   [NSApp stopModal];
}

void AP_CocoaDialog_Spell::event_Cancel()
{
   m_bCancelled = true;
   [NSApp stopModal];
}

void AP_CocoaDialog_Spell::event_SuggestionSelected(int row, int column)
{
	if (!m_Suggestions->getItemCount()) {
		return;
	}
	m_iSelectedRow = row;
	
	[m_dlg setReplace:[[m_suggestionList array] objectAtIndex:row]];
}

void AP_CocoaDialog_Spell::event_ReplacementChanged()
{
	NSString *replace = [m_dlg replace];
	
	int idx = [[m_suggestionList array] indexOfObject:replace];
	[m_dlg selectSuggestion:idx];
	m_iSelectedRow = idx;
}



@implementation AP_CocoaDialog_Spell_Controller

- (id)initFromNib
{
	self = [super initWithWindowNibName:@"ap_CocoaDialog_Spell"];
	return self;
}

-(void)discardXAP
{
	_xap = NULL; 
}

-(void)dealloc
{
	[super dealloc];
}

- (void)setXAPOwner:(XAP_Dialog *)owner
{
	_xap = dynamic_cast<AP_CocoaDialog_Spell*>(owner);
}

-(void)windowDidLoad
{
	if (_xap) {
		const XAP_StringSet *pSS = XAP_App::getApp()->getStringSet();
		LocalizeControl([self window], pSS, AP_STRING_ID_DLG_Spell_SpellTitle);
		LocalizeControl(_cancelBtn, pSS, XAP_STRING_ID_DLG_Cancel);

		LocalizeControl(_addBtn, pSS, AP_STRING_ID_DLG_Spell_AddToDict);
		LocalizeControl(_changeAllBtn, pSS, AP_STRING_ID_DLG_Spell_ChangeAll);
		LocalizeControl(_changeBtn, pSS, AP_STRING_ID_DLG_Spell_Change);
		LocalizeControl(_ignoreAllBtn, pSS, AP_STRING_ID_DLG_Spell_IgnoreAll);
		LocalizeControl(_ignoreBtn, pSS, AP_STRING_ID_DLG_Spell_Ignore);
		LocalizeControl(_replLabel, pSS, AP_STRING_ID_DLG_Spell_ChangeTo);
		LocalizeControl(_unknownLabel, pSS, AP_STRING_ID_DLG_Spell_UnknownWord);
		[_suggestionList setTarget:self];
		[_suggestionList setAction:@selector(suggestionSelected:)];
	}
}


- (IBAction)addToDictAction:(id)sender
{
	_xap->event_AddToDict();
}

- (IBAction)cancelAction:(id)sender
{
	_xap->event_Cancel();
}

- (IBAction)changeAction:(id)sender
{
	_xap->event_Change();
}

- (IBAction)changeAllAction:(id)sender
{
	_xap->event_ChangeAll();
}

- (IBAction)ignoreAction:(id)sender
{
	_xap->event_Ignore();
}

- (IBAction)ignoreAllAction:(id)sender
{
	_xap->event_IgnoreAll();
}

- (IBAction)replacementChanged:(id)sender
{
	_xap->event_ReplacementChanged();
}

- (void)suggestionSelected:(id)sender
{
	_xap->event_SuggestionSelected([sender selectedRow], [sender selectedColumn]);
}

- (void)setMisspelled:(NSAttributedString*)attr
{
	// TODO
	if (attr) {
		[_unknownData setString:[attr string]];
	}
	else {
		[_unknownData setString:@""];
	}
}


- (void)setReplace:(NSString*)str
{
	[_replData setStringValue:str];	
}


- (void)selectSuggestion:(int)idx
{
	[_suggestionList selectRow:idx byExtendingSelection:NO];
}


- (void)setSuggestionList:(id)list
{
	[_suggestionList setDataSource:list];
}

- (NSString*)replace
{
	return [_replData stringValue];
}

@end

