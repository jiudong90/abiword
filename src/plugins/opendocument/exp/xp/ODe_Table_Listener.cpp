/* AbiSource
 * 
 * Copyright (C) 2005 INdT
 * Author: Daniel d'Andrada T. de Carvalho <daniel.carvalho@indt.org.br>
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
 
// Class definition include
#include "ODe_Table_Listener.h"

// Internal includes
#include "ODe_AuxiliaryData.h"
#include "ODe_Common.h"
#include "ODe_ListenerAction.h"
#include "ODe_AutomaticStyles.h"
#include "ODe_Style_Style.h"
#include "ODe_Text_Listener.h"

// AbiWord includes
#include <pp_AttrProp.h>
#include <gsf/gsf-output-memory.h>


/**
 * Constructor
 */
ODe_Table_Listener::ODe_Table_Listener(ODe_AutomaticStyles& rAutomatiStyles,
                                       GsfOutput* pTextOutput,
                                       ODe_AuxiliaryData& rAuxiliaryData,
                                       UT_uint8 zIndex,
                                       UT_uint8 spacesOffset)
                                       :
                                       ODe_AbiDocListenerImpl(spacesOffset),
                                       m_pColumns(NULL),
                                       m_numColumns(0),
                                       m_pRows(NULL),
                                       m_numRows(0),
                                       m_pTextOutput(pTextOutput),
                                       m_rAutomatiStyles(rAutomatiStyles),
                                       m_rAuxiliaryData(rAuxiliaryData),
                                       m_zIndex(zIndex),
                                       m_pTableWideCellStyle(NULL)
{
}


/**
 * Destructor
 */
ODe_Table_Listener::~ODe_Table_Listener() {
    DELETEPV(m_pColumns);
    DELETEPV(m_pRows);
    UT_VECTOR_PURGEALL(ODe_Table_Cell*, m_cells);
    DELETEP(m_pTableWideCellStyle);
}


/**
 * 
 */
void ODe_Table_Listener::openTable(const PP_AttrProp* pAP,
                                   ODe_ListenerAction& rAction) {
    const gchar* pValue;
    bool ok;
    gchar buffer[100];
    const gchar* pVar;
    UT_uint32 i;
    ODe_Style_Style* pStyle;
    UT_UTF8String styleName;
    UT_GenericVector<UT_UTF8String*> columnStyleNames;
    UT_GenericVector<UT_UTF8String*> rowStyleNames;

    m_rAuxiliaryData.m_tableCount++;
    UT_UTF8String_sprintf(m_tableName, "Table%u", m_rAuxiliaryData.m_tableCount);

    if (ODe_Style_Style::hasTableStyleProps(pAP)) {
        m_tableStyleName = m_tableName; // Plain simple
        
        pStyle = m_rAutomatiStyles.addTableStyle(m_tableStyleName);
        pStyle->fetchAttributesFromAbiTable(pAP);
        pStyle = NULL; // We're done with it.
                       // OBS: There's no need to delete it as it will be done
                       //      later by ODe_AutomaticStyles destructor.
    }
    
    if (ODe_Style_Style::hasTableCellStyleProps(pAP)) {
        m_pTableWideCellStyle = new ODe_Style_Style();
        m_pTableWideCellStyle->fetchAttributesFromAbiCell(pAP);
        
        // An OpenDocument table can have a background color.
        // So, there is no need to propagate it to its cells.
        m_pTableWideCellStyle->setTableCellBackgroundColor("");
        
        if (m_pTableWideCellStyle->isEmpty()) {
            // It only had this background color attribute. Now that it's empty
            // There's no reason to use it.
            DELETEP(m_pTableWideCellStyle);
        }
    }

    m_numColumns = 0;
    ok = pAP->getProperty("table-column-props", pValue);
    if (ok && pValue != NULL) {
        pVar = pValue;
        i=0;
        while (*pVar != 0) {
            if (*pVar == '/') {
                buffer[i] = 0; // NULL-terminate the string

                if (buffer[0] != 0 /* or i > 0 */) {

                    UT_UTF8String_sprintf(styleName, "%s.col%u",
                                          m_tableName.utf8_str(), m_numColumns+1);
                                          
                    pStyle = m_rAutomatiStyles.addTableColumnStyle(styleName);
                    pStyle->setColumnWidth(buffer);

                    columnStyleNames.addItem(new UT_UTF8String(styleName));
                    
                    i=0; // Clear the buffer.
                } else {
                    columnStyleNames.addItem(new UT_UTF8String(""));
                }

                m_numColumns++;
            } else {
                buffer[i] = *pVar;
                i++;
            }
            pVar++;
        }
    }

    m_numRows = 0;
    ok = pAP->getProperty("table-row-heights", pValue);
    if (ok && pValue != NULL) {
        pVar = pValue;
        i=0;
        while (*pVar != 0) {
            if (*pVar == '/') {
                buffer[i] = 0; // NULL-terminate the string

                if (buffer[0] != 0 /* or i > 0 */) {

                    UT_UTF8String_sprintf(styleName, "%s.row%u",
                                          m_tableName.utf8_str(), m_numRows+1);

                    pStyle = m_rAutomatiStyles.addTableRowStyle(styleName);
                    pStyle->setRowHeight(buffer);
                    
                    rowStyleNames.addItem(new UT_UTF8String(styleName));

                    i=0; // Clear the buffer.
                } else {
                    rowStyleNames.addItem(new UT_UTF8String(""));
                }
                
                m_numRows++;
            } else {
                buffer[i] = *pVar;
                i++;
            }
            pVar++;
        }
    }
    
    
    if (m_numColumns > 0) {
        m_pColumns = new ODe_Table_Column[m_numColumns];
        
        for (i=0; i<m_numColumns; i++) {
            m_pColumns[i].m_styleName = *(columnStyleNames[i]);
        }
    }
    
    if (m_numRows > 0) {
        m_pRows = new ODe_Table_Row[m_numRows];
        
        for (i=0; i<m_numRows; i++) {
            m_pRows[i].m_styleName = *(rowStyleNames[i]);
        }
    }
    
    
    UT_VECTOR_PURGEALL(UT_UTF8String*, columnStyleNames);
    UT_VECTOR_PURGEALL(UT_UTF8String*, rowStyleNames);
}


/**
 * 
 */
void ODe_Table_Listener::closeTable(ODe_ListenerAction& rAction) {
    UT_uint32 i;
    UT_UTF8String output;
    
    _buildTable();
    
    _printSpacesOffset(output);
    output += "<table:table table:name=\"";
    output += m_tableName;
    output += "\"";

    ODe_writeAttribute(output, "table:style-name", m_tableStyleName);
    output += ">\n";
    
    ODe_writeToFile(m_pTextOutput, output);
    m_spacesOffset++;

    
    output.clear();
    _printSpacesOffset(output);
    
    for (i=0; i<m_numColumns; i++) {
        m_pColumns[i].write(m_pTextOutput, output);
    }
    
    for (i=0; i<m_numRows; i++) {
        m_pRows[i].write(m_pTextOutput, output);
    }


    output.clear();
    m_spacesOffset--;
    _printSpacesOffset(output);
    output += "</table:table>\n";
    ODe_writeToFile(m_pTextOutput, output);

    rAction.popListenerImpl();
}


/**
 * 
 */
void ODe_Table_Listener::openCell(const PP_AttrProp* pAP,
                                  ODe_ListenerAction& rAction) {
                                    
    ODe_Table_Cell* pCell;
    ODe_Text_Listener* pTextListener;
    ODe_Style_Style* pCellStyle;


    // Create the table cell.
    pCell = new ODe_Table_Cell();
    m_cells.addItem(pCell);
    
    pCell->loadAbiProps(pAP);

    ////    
    // Try to figure out the table dimensions.
    
    // If this cell is in the rightmost column than we have found the number
    // of columns. If it's not, it will do no harm.
    if (m_numColumns < pCell->m_rightAttach) {
        m_numColumns = pCell->m_rightAttach;
    }
    
    // If this cell is in the last row than we have found the number
    // of rows. If it's not, it will do no harm.
    if (m_numRows < pCell->m_bottomAttach) {
        m_numRows = pCell->m_bottomAttach;
    }
    
    
    ////
    // Define its style
    
    if (ODe_Style_Style::hasTableCellStyleProps(pAP) ||
        m_pTableWideCellStyle != NULL) {
    
        UT_UTF8String_sprintf(pCell->m_styleName, "%s_col%u_row%u",
                              m_tableName.utf8_str(),
                              (pCell->m_leftAttach)+1,
                              (pCell->m_topAttach)+1);
                              
        pCellStyle = m_rAutomatiStyles.addTableCellStyle(pCell->m_styleName);
        
        if (m_pTableWideCellStyle != NULL) {
            *pCellStyle = *m_pTableWideCellStyle;
        }
        
        pCellStyle->fetchAttributesFromAbiCell(pAP);
    }
    

    ////
    // Prepare to read its contents
    
    pCell->m_pTextContent = gsf_output_memory_new ();
    UT_ASSERT(pCell->m_pTextContent != NULL);
    
    pTextListener = new ODe_Text_Listener(m_rAutomatiStyles,
        pCell->m_pTextContent,
        m_rAuxiliaryData,
        m_zIndex,
        // currentOffset + <table:table> + <table:table-row> + <table:table-cell>
        m_spacesOffset+3);
    rAction.pushListenerImpl(pTextListener, true);
}


/**
 * 
 */
void ODe_Table_Listener::_buildTable() {
    
    UT_uint32 i, j;
    ODe_Table_Cell* pCell;
    
    UT_ASSERT(m_numRows > 0);
    UT_ASSERT(m_numColumns > 0);
    
    // Create the columns, if not done already
    if (m_pColumns == NULL) {
        m_pColumns = new ODe_Table_Column[m_numColumns];
    }
    
    // Create the rows, if not done already
    if (m_pRows == NULL) {
        m_pRows = new ODe_Table_Row[m_numRows];
    }
    
    // Create the vectors that will hold the cells into its corresponding rows.
    for (i=0; i<m_numRows; i++) {
        m_pRows[i].m_ppCells = new ODe_Table_Cell*[m_numColumns];
        m_pRows[i].m_columnCount = m_numColumns;
        
        for (j=0; j<m_numColumns; j++) {
            m_pRows[i].m_ppCells[j] = NULL;
        }
    }
    
    // Position cells in table
    for (i=0; i<m_cells.getItemCount(); i++) {
        pCell = m_cells.getNthItem(i);

        UT_ASSERT(pCell->m_topAttach < m_numRows);
        UT_ASSERT(pCell->m_leftAttach < m_numColumns);
        
        m_pRows[pCell->m_topAttach].m_ppCells[pCell->m_leftAttach] = pCell;
    }
}


/**
 * 
 */
void ODe_Table_Cell::loadAbiProps(const PP_AttrProp* pAP) {
    const gchar* pValue;
    bool ok;
    
    ok = pAP->getProperty("left-attach", pValue);
    if (!ok || pValue == NULL) {
        UT_ASSERT(UT_SHOULD_NOT_HAPPEN);
    }
    m_leftAttach = atoi(pValue);
    
    ok = pAP->getProperty("right-attach", pValue);
    if (!ok || pValue == NULL) {
        UT_ASSERT(UT_SHOULD_NOT_HAPPEN);
    }
    m_rightAttach = atoi(pValue);

    ok = pAP->getProperty("top-attach", pValue);
    if (!ok || pValue == NULL) {
        UT_ASSERT(UT_SHOULD_NOT_HAPPEN);
    }
    m_topAttach = atoi(pValue);

    ok = pAP->getProperty("bot-attach", pValue);
    if (!ok || pValue == NULL) {
        UT_ASSERT(UT_SHOULD_NOT_HAPPEN);
    }
    m_bottomAttach = atoi(pValue);

    // A few sanity checks
    UT_ASSERT(m_leftAttach   >= 0);
    UT_ASSERT(m_rightAttach  >= 0);
    UT_ASSERT(m_topAttach    >= 0);
    UT_ASSERT(m_bottomAttach >= 0);
    UT_ASSERT(m_leftAttach   <  m_rightAttach);
    UT_ASSERT(m_topAttach    <  m_bottomAttach);
    
    
    ////
    // Define some cell properties

    if (m_rightAttach - m_leftAttach > 1) {
        UT_UTF8String_sprintf(m_numberColumnsSpanned, "%d",
                              m_rightAttach-m_leftAttach);
    }
    
    if (m_bottomAttach - m_topAttach > 1) {
        UT_UTF8String_sprintf(m_numberRowsSpanned, "%d",
                              m_bottomAttach - m_topAttach);
    }
}


/**
 * 
 */
void ODe_Table_Cell::write(GsfOutput* pTableOutput, const UT_UTF8String& rSpacesOffset) {
    UT_UTF8String output;
    
    output = rSpacesOffset;
    output += "<table:table-cell";
    ODe_writeAttribute(output, "table:style-name", m_styleName);
    output += ">\n";
    ODe_writeToFile(pTableOutput, output);
    
    gsf_output_write (pTableOutput, gsf_output_size (m_pTextContent),
		      gsf_output_memory_get_bytes (GSF_OUTPUT_MEMORY (m_pTextContent)));

    output = rSpacesOffset;
    output += "</table:table-cell>\n";
    ODe_writeToFile(pTableOutput, output);
}


/**
 * 
 */
void ODe_Table_Column::write(GsfOutput* pTableOutput, const UT_UTF8String& rSpacesOffset) {
    UT_UTF8String output;
    
    output = rSpacesOffset;
    output += "<table:table-column";
    ODe_writeAttribute(output, "table:style-name", m_styleName);
    output += "/>\n";
                          
    ODe_writeToFile(pTableOutput, output);
}


/**
 * 
 */
void ODe_Table_Row::write(GsfOutput* pTableOutput, const UT_UTF8String& rSpacesOffset) {
    UT_UTF8String output;
    UT_UTF8String cellsOffset;
    UT_uint32 i;
    
    output = rSpacesOffset;
    output += "<table:table-row";
    ODe_writeAttribute(output, "table:style-name", m_styleName);
    output += ">\n";
    
    ODe_writeToFile(pTableOutput, output);
    
    cellsOffset = rSpacesOffset;
    cellsOffset += " ";
    
    for (i=0; i<m_columnCount; i++) {
        if (m_ppCells[i] != NULL) {
            m_ppCells[i]->write(pTableOutput, cellsOffset);
        } else {
            // It's a covered cell.
            output = cellsOffset;
            output += "<table:covered-table-cell/>\n";
            ODe_writeToFile(pTableOutput, output);
        }
    }

    output = rSpacesOffset;
    output += "</table:table-row>\n";
    ODe_writeToFile(pTableOutput, output);
}


/**
 * 
 */
ODe_Table_Row::ODe_Table_Row() 
	: m_ppCells(NULL),
	m_columnCount(0)
{
}


/**
 * 
 */
ODe_Table_Row::~ODe_Table_Row() {
    DELETEPV(m_ppCells);
}
