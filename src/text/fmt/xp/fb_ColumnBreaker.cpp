/* AbiWord
 * Copyright (C) 1998 AbiSource, Inc.
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

#include "fb_ColumnBreaker.h"
#include "fl_SectionLayout.h"
#include "fp_ContainerObject.h"
#include "fp_TableContainer.h"
#include "fl_BlockLayout.h"
#include "fp_Line.h"
#include "fp_Column.h"
#include "ut_assert.h"

fb_ColumnBreaker::fb_ColumnBreaker()
{
}

/*!
  Layout sections on pages
  \return zero

  This is the function that controls how sections and thereby columns,
  blocks, and lines are laid out on the pages. Doing so it refers to
  the various layout configurations such as orphan/widow controls and
  break Runs embedded in the text.
*/
UT_sint32 fb_ColumnBreaker::breakSection(fl_DocSectionLayout * pSL)
{
	bool bHasFootnote = false, bFirstColumn = true;

	fl_ContainerLayout* pFirstLayout = NULL;
	fp_Container* pCurrentContainer = NULL;
	fp_Column* pCurColumn = NULL;

	pFirstLayout = pSL->getFirstLayout();
	if (!pFirstLayout)
		return 0;
	pCurrentContainer = pFirstLayout->getFirstContainer();
	pCurColumn = (fp_Column*) pSL->getFirstContainer();

	while (pCurColumn)
	{
		fp_Container* pFirstContainerToKeep = pCurrentContainer;
		fp_Container* pLastContainerToKeep = NULL;
		fp_Container* pOffendingContainer = NULL;
#ifndef WITH_PANGO
		UT_sint32 iMaxSecCol = pSL->getMaxSectionColumnHeightInLayoutUnits();
 		UT_sint32 iMaxColHeight = pCurColumn->getMaxHeightInLayoutUnits();
#else
		UT_sint32 iMaxSecCol = pSL->getMaxSectionColumnHeight();
 		UT_sint32 iMaxColHeight = pCurColumn->getMaxHeight();
#endif
		bool bEquivColumnBreak = false;
		xxx_UT_DEBUGMSG(("SEVIOR: iMaxSecCol = %d iMaxColHeight = %d \n",iMaxSecCol,iMaxColHeight));
		if((iMaxSecCol > 0) && (iMaxSecCol < iMaxColHeight))
		{
			iMaxColHeight = iMaxSecCol;
		    bEquivColumnBreak = true;
		}
		UT_sint32 iWorkingColHeight = 0;

		fp_Container* pCurContainer = pFirstContainerToKeep;
		if (pCurContainer && pCurContainer->getContainerType() == FP_CONTAINER_LINE)
		{
			fp_Line* pCurLine = (fp_Line *)pCurContainer;
			bHasFootnote |= pCurLine->containsFootnoteRef();
			// Excellent.  If we have a footnote, we can start deducting
			// from the working height before we lay out the text.
			if (pCurLine->containsFootnoteRef())
			{
				// Unless bFirstColumn is false.  In which case we assert
				// NOT_YET_IMPLEMENTED for now!  Hurrah!
				if (!bFirstColumn)
					UT_ASSERT(UT_NOT_IMPLEMENTED);

				// Ok.  Now, deduct the proper amount from iMaxColHeight.
				// We need to get the footnote section.
				// what's the proper place to do that?
			}
		}

		// Special handling of columns that should be skipped due to
		// page breaks. If the previous line contains a page break,
		// skip the present column if it is on the same page.
		if (pCurContainer)
		{
			fp_Container* pPrevContainer = (fp_Container *) pCurContainer->getPrev();
			if(pPrevContainer && pPrevContainer->getContainerType() == FP_CONTAINER_LINE)
			{
				fp_Line * pL = (fp_Line *) pPrevContainer;
				{
					if (pL->containsForcedPageBreak()
				&& (pCurColumn->getPage() == pL->getContainer()->getPage()))
					{
						pCurColumn = (fp_Column *) pCurColumn->getNext();
						continue;
					}
				}
			}
		}
		bool bBreakOnColumnBreak = false;
		bool bBreakOnPageBreak = false;
		UT_sint32  iTotalContainerSpace = 0;
		bool bDoTableBreak = false;
		while (pCurContainer)
		{
			UT_sint32 iContainerHeight =0;
#ifndef WITH_PANGO
			if(pCurContainer->getContainerType() == FP_CONTAINER_TABLE)
			{
				iContainerHeight = static_cast<fp_TableContainer *>(pCurContainer)->getHeightInLayoutUnits();
			}
			else
			{
				iContainerHeight = pCurContainer->getHeightInLayoutUnits();
			}
			UT_sint32 iContainerMarginAfter = pCurContainer->getMarginAfterInLayoutUnits();
#else
			if(pCurContainer->getContainerType() == FP_CONTAINER_TABLE)
			{
				iContainerHeight = static_cast<fp_TableContainer *>(pCurContainer)->getHeight();
			}
			else
			{
				iContainerHeight = pCurContainer->getHeight();
			}
			UT_sint32 iContainerMarginAfter = pCurContainer->getMarginAfter();
#endif
			iTotalContainerSpace = iContainerHeight + iContainerMarginAfter;

			if ((iWorkingColHeight + iTotalContainerSpace) > iMaxColHeight )
			{
				pOffendingContainer = pCurContainer;
				bDoTableBreak = false;

				/*
				  We have found the offending container (the first one
				  which won't fit in the
				  column) and we now need to decide whether we can break
				  the column just before it.  */
//
// Now we check to see if the offending container is a table and if it
// can be broken to fit in the column.
//

				xxx_UT_DEBUGMSG(("SEVIOR: iWorkingColHeight %d iTotalContainerSpace %d iMaxColHeight %d pCurContainer %x height %d \n",iWorkingColHeight,iTotalContainerSpace,iMaxColHeight,   pCurContainer,  iContainerHeight));
				if(pOffendingContainer->getContainerType() == FP_CONTAINER_TABLE)
				{
					xxx_UT_DEBUGMSG(("SEVIOR: Offending container is table %x \n",pOffendingContainer));
					fp_TableContainer * pTab = (fp_TableContainer *) pOffendingContainer;
					if(!pTab->isThisBroken())
					{
//
// This is the first of this table set. Clear the old broken tables
// and rebreak.
//
						pTab->deleteBrokenTables();
						bDoTableBreak = true;
						xxx_UT_DEBUGMSG(("SEVIOR: Need Table Break 1 \n"));
						xxx_UT_DEBUGMSG(("firstbroke %x lastbroke %x \n",pTab->getFirstBrokenTable(),pTab->getLastBrokenTable()));
					}
					else
					{
//
// Find the right broken table to bump.
//
//
#if 0
						if( pFirstContainerToKeep == pOffendingContainer)
						{
							if(pTab->getNext() == NULL)
							{
								bDoTableBreak = true;
								UT_DEBUGMSG(("SEVIOR: Need Table Break 2 \n"));
							}
							else
							{
								pOffendingContainer = (fp_Container *) pTab->getNext();
							}
						}
						else
						{
//
// We bump the broken table after this one and trust the adjustTablesize
// method to set things right.
//
							if(pTab->getNext())
							{
								pOffendingContainer = (fp_Container *) pTab->getNextContainerInSection();
							}
//
// No following table! So we need to break this.
//
							else
							{
								bDoTableBreak = true;
							}
						}
#endif
						bDoTableBreak = true;
					}
//
// If we don't break the table, the heights of the broken table's
// will be adjusted at the setY() in the layout stage.
//
					fp_TableContainer * pBroke = NULL;
					UT_sint32 iAvail = iMaxColHeight - iWorkingColHeight - iContainerMarginAfter;
					double scale = ((double) pTab->getGraphics()->getResolution())/UT_LAYOUT_UNITS;
					iAvail = (UT_sint32)( ((double) iAvail)*scale);
					UT_sint32 iBreakAt = pTab->wantVBreakAt(iAvail -1);
					if(iBreakAt > (iAvail -1))
					{
						UT_ASSERT(UT_SHOULD_NOT_HAPPEN);
					}
					UT_sint32 iBreakLO = (UT_sint32) (((double) iBreakAt)/scale);
					if(bDoTableBreak && (iBreakLO + iWorkingColHeight <= iMaxColHeight))
					{
//
// OK we can break this table and keep some of it in this column. The
// rest goes into the next column.
//
// Look to see if this table is broken.
//
						if(!pTab->isThisBroken())
						{
//
// Break it at 0 first.
//
							UT_DEBUGMSG(("SEVIOR: Breaking MAster iBreakAt %d yloc = %d \n",iBreakAt,pTab->getY()));
							UT_DEBUGMSG(("SEVIOR: iBreakLO %d iWorkingColHeight %d iMaxColHeight %d Container Height %d MArginAfter %d \n",iBreakLO,iWorkingColHeight,iMaxColHeight,pTab->getHeightInLayoutUnits() , iContainerMarginAfter ));
							fp_Container * pNext = (fp_Container *) pTab->getNext();
							UT_DEBUGMSG(("SEVIOR: getNext %x \n",pNext));
							if(pNext)
							{
								UT_DEBUGMSG(("SEVIOR: Container of next %d \n",pNext->getContainerType()));
							}
							pTab->deleteBrokenTables();
							pTab->VBreakAt(0);
						}
//
// Now get a broken table and break it again.
//
						if(!pTab->isThisBroken())
						{
							pBroke = pTab->getFirstBrokenTable();
						}
						else
						{
							pBroke = pTab;
						}
//
// Look to see if we have to move the table out of this container.
//
						if(iBreakAt < 30 )
						{
							pOffendingContainer = pTab;
							pCurContainer = pTab;
						}
						else
						{
//
// When we break the table, the bit broken will be placed after the current
// table in the column. This then becomes the offending container and will
// be bumped into the next column. Pretty cool eh ? :-)
//
							pOffendingContainer = (fp_Container *) pBroke->VBreakAt(iBreakAt);
							xxx_UT_DEBUGMSG(("SEVIOR: Created broken table %x \n",pOffendingContainer));
							pCurContainer = pOffendingContainer;
							pLastContainerToKeep = (fp_Container *) pTab;
							xxx_UT_DEBUGMSG(("SEVIOR: Set lasttokeep 1 %x \n",pLastContainerToKeep));
						}
					}
				}
				if (pOffendingContainer == pFirstContainerToKeep)
				{
					// Wow!  The very first line in this column won't
					// fit.  Big line.  (or maybe a small column)
					// TODO: what should we do here?  For now, we
					// force it.

					pLastContainerToKeep = pFirstContainerToKeep;
					xxx_UT_DEBUGMSG(("SEVIOR: Set lasttokeep 2 %x \n",pLastContainerToKeep));
				}
				else
				{
					UT_uint32 iWidows = 0;
					UT_uint32 iOrphans = 0;
					bool bIsTable = false;
					fl_BlockLayout* pBlock = NULL;
					fl_ContainerLayout * pConLayout = NULL;
					if(pOffendingContainer->getContainerType() == FP_CONTAINER_LINE)
					{
						pBlock = static_cast<fp_Line *>(pOffendingContainer)->getBlock();
						iWidows = pBlock->getProp_Widows();
						iOrphans = pBlock->getProp_Orphans();
						pConLayout = (fl_ContainerLayout *) pBlock;
					}
					else
					{
						bIsTable = true;
						pConLayout = (fl_ContainerLayout *) pOffendingContainer->getSectionLayout();
					}
					if(bIsTable)
					{

						pLastContainerToKeep = pOffendingContainer->getPrevContainerInSection();
						xxx_UT_DEBUGMSG(("SEVIOR: Set lasttokeep 3 %x \n",pLastContainerToKeep));
						UT_ASSERT(pLastContainerToKeep);
//
// All this deals with widows and orphans. We don't need this for
// tables
//
						break;
					}
					UT_uint32 iNumContainersBeforeOffending = 0;
					UT_uint32 iNumContainersAfterOffending = 0;
					bool bFoundOffending = false;
					fp_Container* pFirstContainerInBlock = pConLayout->getFirstContainer();
					pCurContainer = pFirstContainerInBlock;
					while (pCurContainer)
					{
						if (bFoundOffending)
						{
							iNumContainersAfterOffending++;
						}
						else
						{
							if (pCurContainer == pOffendingContainer)
							{
								iNumContainersAfterOffending = 1;
								bFoundOffending = true;
							}
							else
							{
								iNumContainersBeforeOffending++;
							}
						}
						pCurContainer = (fp_Container *) pCurContainer->getNext();
					}

					UT_uint32 iNumContainersInLayout = iNumContainersBeforeOffending + iNumContainersAfterOffending;

					UT_uint32 iNumLayoutContainersInThisColumn = 0;
					pCurContainer = (fp_Container *) pOffendingContainer->getPrev();
					while (pCurContainer)
					{
						iNumLayoutContainersInThisColumn++;
						if (pCurContainer == pFirstContainerToKeep)
						{
							break;
						}

						pCurContainer = (fp_Container *) pCurContainer->getPrev();
					}
					bool bKeepBlock = false;
					if ( pBlock &&
						pBlock->getProp_KeepTogether()
						&& (iNumContainersBeforeOffending == iNumLayoutContainersInThisColumn)
						&& (pBlock->getFirstContainer() != pFirstContainerToKeep)
						)
					{
						bKeepBlock = true;
					}
					if(bKeepBlock)
					{
						/*
						  This block or Table wants to be kept all in the same column.
						  Bump the whole block to the next column.
						*/

						/*
						  If the block is simply too big to fit in a
						  single column, then we can spawn an infinite
						  loop by continually punting it to the next
						  one.  So, we assume that if the first line
						  in the block is the first line in this
						  column, we just keep it and cope.  This will
						  be slightly incorrect in cases where pushing
						  it to the next column would allow the block
						  to try to live in a larger column, thus
						  staying all together.
						*/

						pLastContainerToKeep = (fp_Container *) pFirstContainerInBlock->getPrevContainerInSection();
						xxx_UT_DEBUGMSG(("SEVIOR: Set lasttokeep 4 %x \n",pLastContainerToKeep));
					}
					else if (
						(iNumContainersInLayout < (iWidows + iOrphans))
						&& (iNumContainersBeforeOffending == iNumLayoutContainersInThisColumn)
						)
					{
						/*
						  There are not enough lines to divide between the
						  two columns while still satisfying both constraints.
						  Bump the whole block to the next column.
						*/

						pLastContainerToKeep = (fp_Container *) pFirstContainerInBlock->getPrevContainerInSection();
//
// If this happens to be a table better get the LAST of the broken tables
//
						if(pLastContainerToKeep->getContainerType() == FP_CONTAINER_TABLE)
						{
							//
							// Can get into trouble if the pFirstContainer
							// points to a broken table after
							// this. Check for this.
							//
							if(pFirstContainerToKeep->getContainerType() == FP_CONTAINER_TABLE)
							{
								fp_TableContainer * pTab = (fp_TableContainer *) pLastContainerToKeep;
								bool bFound = false;
								while(pTab && !bFound)
								{
									if(static_cast<fp_Container *>(pTab) == pFirstContainerToKeep)
									{
										bFound = true;
									}
									else
									{
										pTab = (fp_TableContainer *) pTab->getNext();
									}
								}
								if(bFound)
								{
									pFirstContainerToKeep = pLastContainerToKeep;
								}
							}
						}
						xxx_UT_DEBUGMSG(("SEVIOR: Set lasttokeep 5 %x \n",pLastContainerToKeep));
					}
					else if (
						(iNumContainersBeforeOffending < iOrphans)
						&& (iNumContainersBeforeOffending == iNumLayoutContainersInThisColumn)
						)
					{
						/*
						  We're leaving too few lines in the current column.
						  Bump the whole block.
						*/

						pLastContainerToKeep = (fp_Container *) pFirstContainerInBlock->getPrevContainerInSection();
						xxx_UT_DEBUGMSG(("SEVIOR: Set lasttokeep 6 %x \n",pLastContainerToKeep));
					}
					else if (
						(iNumContainersAfterOffending < iWidows)
						&& ((iWidows - iNumContainersAfterOffending) < iNumLayoutContainersInThisColumn)
						)
					{
						/*
						  There aren't going to be enough lines in the next
						  column.  Bump just enough.
						*/

						UT_uint32 iNumContainersNeeded = (iWidows - iNumContainersAfterOffending);

						pLastContainerToKeep = (fp_Container *) pOffendingContainer->getPrevContainerInSection();
						xxx_UT_DEBUGMSG(("SEVIOR: Set lasttokeep 7 %x \n",pLastContainerToKeep));
						for (UT_uint32 iBump = 0; iBump < iNumContainersNeeded; iBump++)
						{

							pLastContainerToKeep = (fp_Container *) pLastContainerToKeep->getPrevContainerInSection();
							xxx_UT_DEBUGMSG(("SEVIOR: Set lasttokeep 8 %x \n",pLastContainerToKeep));
						}
					}
					else
					{

						pLastContainerToKeep = (fp_Container *) pOffendingContainer->getPrevContainerInSection();
						xxx_UT_DEBUGMSG(("SEVIOR: Set lasttokeep 8 %x \n",pLastContainerToKeep));
					}
				}
				break;
			}
			else
			{
				iWorkingColHeight += iTotalContainerSpace;
				if(pCurContainer->getContainerType() == FP_CONTAINER_LINE)
				{
					fp_Line * pL = (fp_Line *) pCurContainer;
					if (
						pL->containsForcedColumnBreak()
						|| pL->containsForcedPageBreak()
						)
					{

						pLastContainerToKeep = pCurContainer;
						xxx_UT_DEBUGMSG(("SEVIOR: Set lasttokeep 9 %x \n",pLastContainerToKeep));
						bBreakOnColumnBreak = ( pL->containsForcedColumnBreak()) ;
						bBreakOnPageBreak = pL->containsForcedPageBreak();
						if((iWorkingColHeight >=  iMaxColHeight))
							bBreakOnColumnBreak = false;
						break;
					}
				}
			}

			pCurContainer = (fp_Container *) pCurContainer->getNextContainerInSection();
			if (pCurContainer && pCurContainer->getContainerType() == FP_CONTAINER_LINE)
			{
				fp_Line* pCurLine = (fp_Line *)pCurContainer;
				bHasFootnote |= pCurLine->containsFootnoteRef();
				// Excellent.  If we have a footnote, we can start deducting
				// from the working height before we lay out the text.
				if (pCurLine->containsFootnoteRef())
				{
					// Unless bFirstColumn is false.  In which case we assert
					// NOT_YET_IMPLEMENTED for now!  Hurrah!
					if (!bFirstColumn)
						UT_ASSERT(UT_NOT_IMPLEMENTED);

					// Ok.  Now, deduct the proper amount from iMaxColHeight.
					// We need to get the footnote section.
					// what's the proper place to do that?
				}
			}
		}
//
// End of big while loop here. After this we've found LastContainerToKeep
//
		bEquivColumnBreak = bEquivColumnBreak && ( iMaxColHeight < (iWorkingColHeight + iTotalContainerSpace));
		if (pLastContainerToKeep)
		{
			pCurrentContainer = (fp_Container *) pLastContainerToKeep->getNextContainerInSection();
		}
		else
		{
			pCurrentContainer = NULL;
		}
//
// OK fill our column with content between pFirstContainerToKeep and pLastContainerToKeep
//
		pCurContainer = pFirstContainerToKeep;
		while (pCurContainer)
		{
			if (pCurContainer->getContainer() != pCurColumn)
			{
				static_cast<fp_VerticalContainer *>(pCurContainer->getContainer())->removeContainer(pCurContainer);
				pCurColumn->addContainer(pCurContainer);
			}

			if (pCurContainer == pLastContainerToKeep)
			{
				break;
			}
			else
			{
				if(pLastContainerToKeep)
				{
					UT_ASSERT(pCurContainer->getNextContainerInSection());
				}
				pCurContainer = (fp_Container *) pCurContainer->getNextContainerInSection();
			}
		}

		if (pLastContainerToKeep
			&& static_cast<fp_Container *>(pCurColumn->getLastContainer()) != pLastContainerToKeep)
		{
			UT_ASSERT(static_cast<fp_Column *>(pLastContainerToKeep->getColumn()) == pCurColumn);

			fp_Page* pPrevPage = pCurColumn->getPage();

			fp_Column* pNextColumn = pCurColumn;
			do
			{
				// Make sure there is a next column and that it
				// falls on the next page if there's a page break.
				pNextColumn = (fp_Column *) pNextColumn->getNext();
				if(bBreakOnColumnBreak || bEquivColumnBreak)
				{
					if((pNextColumn != NULL) &&( pNextColumn != pCurColumn->getFollower()) && (pNextColumn->getPage() != pCurColumn->getPage()))
					{
						pNextColumn = NULL;
					}
				}
				if (!pNextColumn)
				{
					if(bBreakOnColumnBreak || bEquivColumnBreak)
					{
						pNextColumn = (fp_Column*) pSL->getNewContainer( (fp_Container *)pLastContainerToKeep->getNextContainerInSection());
					}
					else
					{
						pNextColumn = (fp_Column*) pSL->getNewContainer(NULL);
					}
				}
			}
			while (pLastContainerToKeep->getContainerType()
				   == FP_CONTAINER_LINE &&
				   static_cast<fp_Line *>(pLastContainerToKeep)->containsForcedPageBreak()
				   && (pNextColumn->getPage() == pPrevPage));
			// Bump content down the columns
			while (pCurColumn != NULL && pCurColumn != pNextColumn)
			{
				pCurColumn->bumpContainers(pLastContainerToKeep);
				pCurColumn->layout();
//
// Layout might delete a broken table. Check for this.
//
				fp_Container * pCon = pCurColumn->getLastContainer();
				if(pCon && pCon->getContainerType() == FP_CONTAINER_TABLE)
				{
					pCurrentContainer = pCon->getNextContainerInSection();
				}
				pCurColumn = (fp_Column *) pCurColumn->getNext();

					// This is only relevant for the initial column. All
					// other columns should flush their entire content.
				pLastContainerToKeep = NULL;
			}

		}
		else
		{
			UT_ASSERT((!pLastContainerToKeep) || (static_cast<fp_Container *>(pCurColumn->getLastContainer()) == pLastContainerToKeep));

			bool bTableTest = false;
			if(pCurrentContainer)
			{
				bTableTest = (pCurrentContainer->getContainerType() == FP_CONTAINER_TABLE);
			}
			pCurColumn->layout();
//
// Layout might delete a broken table. Check for this.
//
			if(pCurrentContainer && bTableTest)
			{
				if(pCurrentContainer != pCurColumn->getLastContainer())
				{
					pCurrentContainer = pCurColumn->getLastContainer()->getNextContainerInSection();
				}
			}
			pCurColumn = (fp_Column *) pCurColumn->getNext();
		}
	}

	return 0; // TODO return code
}
