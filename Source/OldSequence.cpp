/*
** Dn-FamiTracker - NES/Famicom sound tracker
** Copyright (C) 2020-2025 D.P.C.M.
** FamiTracker Copyright (C) 2005-2020 Jonathan Liss
** 0CC-FamiTracker Copyright (C) 2014-2018 HertzDevil
**
** This program is free software: you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation, either version 3 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program. If not, see https://www.gnu.org/licenses/.
*/


#include "stdafx.h"
#include <vector>
// #include "ModuleException.h"
#include "Sequence.h"
#include "OldSequence.h"

COldSequence::COldSequence() : Length(), Value() {}

void COldSequence::AddItem(char len, char val)
{
	Length.push_back(len);
	Value.push_back(val);
}

unsigned int COldSequence::GetLength() const
{
	return static_cast<unsigned int>(Length.size());
}

// // // moved from CFamiTrackerDoc::ConvertSequence

CSequence *COldSequence::Convert(int Type) const
{
	const int Count = GetLength();
	if (Count == 0 || Count >= MAX_SEQUENCE_ITEMS)
		return nullptr;
	
	int iLoopPoint = -1;
	int iLength = 0;
	int ValPtr = 0;

	CSequence *pSeq = new CSequence();

	for (int i = 0; i < Count; ++i) {
		if (Length[i] < 0) {
			iLoopPoint = 0;
			for (int x = Count + Length[i] - 1; x < Count - 1; x++)
				iLoopPoint += (Length[x] + 1);
		}
		else {
			for (int l = 0; l < Length[i] + 1; l++) {
				pSeq->SetItem(ValPtr++, (Type == SEQ_PITCH || Type == SEQ_HIPITCH) && l ? 0 : Value[i]);
				iLength++;
			}
		}
	}

	if (iLoopPoint != -1) {
		if (iLoopPoint > iLength)
			iLoopPoint = iLength;
		iLoopPoint = iLength - iLoopPoint;
	}

	pSeq->SetItemCount(ValPtr);
	pSeq->SetLoopPoint(iLoopPoint);

	/*
	if (Type == SEQ_PITCH || Type == SEQ_HIPITCH) {		// // // (not how they work)
		if (iLoopPoint != -1) {
			pSeq->SetItemCount(++ValPtr);
			pSeq->SetItem(ValPtr, pSeq->GetItem(iLoopPoint));
			pSeq->SetLoopPoint(++iLoopPoint);
		}
		for (int i = ValPtr - 1; i > 0; --i)
			pSeq->SetItem(i, pSeq->GetItem(i) - pSeq->GetItem(i - 1));
	}
	*/

	return pSeq;
}
