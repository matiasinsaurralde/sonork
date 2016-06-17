#include "srkwin32app.h"
#include "sonork.rh"
#pragma hdrstop
#include "srktreeview.h"

struct TSonorkTrackerSubs
{
	DWORD			flags;
	TSonorkTrackerData	data;

};
#define ITEM_STATE_FLAGS_SONORK_TREE_IS_AWARE_OF (TVIS_BOLD | TVIS_STATEIMAGEMASK)
void
 TSonorkTreeView::SetHandle(HWND phwnd)
{
	hwnd=phwnd;
	SetImageList(sonork_skin.Icons());
	SetStyle(TVS_INFOTIP,TVS_HASLINES|TVS_LINESATROOT|WS_BORDER|TVS_INFOTIP);
	SetIndent( 8 );
	UpdateSkin();
}
void
 TSonorkTreeView::UpdateSkin() const
 {
	SendMessage(	  TVM_SETTEXTCOLOR
			, 0
			, sonork_skin.Color(SKIN_COLOR_MAIN,SKIN_CI_FG));
	SendMessage(	  TVM_SETBKCOLOR
			, 0
			, sonork_skin.Color(SKIN_COLOR_MAIN,SKIN_CI_BG));
	SendMessage(	  WM_SETFONT
			, (WPARAM)sonork_skin.Font(SKIN_FONT_MAIN_TREE)
			, 0);
}
void
 TSonorkTreeView::DelAllItems() const
{
	TreeView_DeleteAllItems( hwnd );
}

HTREEITEM
 TSonorkTreeView::GetParent( HTREEITEM hItem ) const
{
	return TreeView_GetParent( hwnd , hItem );
}


TSonorkTreeItem*
 TSonorkTreeView::GetChild( TSonorkTreeItem* VI) const
{
	return VI==NULL
		?NULL
		:GetChild( VI->hitem );
}

TSonorkTreeItem*
 TSonorkTreeView::GetSelectedItem() const
{
	return GetItem( GetSelection() );
}

HTREEITEM
 TSonorkTreeView::GetSelection() const
{
	return TreeView_GetSelection(hwnd);
}

TSonorkTreeItem*
 TSonorkTreeView::GetChild( HTREEITEM hItem ) const
{
	return hItem==NULL
		?NULL
		:GetItem( TreeView_GetChild(hwnd,hItem) );
}

TSonorkTreeItem*
 TSonorkTreeView::GetNextSibling( TSonorkTreeItem* VI) const
{
	return VI==NULL
		?NULL
		:GetNextSibling(VI->hitem);
}

TSonorkTreeItem*
 TSonorkTreeView::GetNextSibling( HTREEITEM hItem ) const
{
	return hItem==NULL
		?NULL
		:GetItem( TreeView_GetNextSibling(hwnd,hItem) );
}



TSonorkTreeItem*
 TSonorkTreeView::GetItem(HTREEITEM hItem) const
{
	TV_ITEM	tv_item;
	if(hItem!=NULL&&hItem!=TVI_ROOT)
	{
		tv_item.hItem		= hItem;
		tv_item.mask		= TVIF_HANDLE | TVIF_PARAM ;
		if( TreeView_GetItem(hwnd,&tv_item))
			return (TSonorkTreeItem*)tv_item.lParam;
	}
	return NULL;
}

void
 TSonorkTreeView::SetImageList(HIMAGELIST himl) const
{
	TreeView_SetImageList(hwnd,himl,TVSIL_NORMAL);
	TreeView_SetImageList(hwnd,himl,TVSIL_STATE );
}
void
 TSonorkTreeView::SetIndent(int v) const
{
	TreeView_SetIndent(hwnd,v);
}


void
 TSonorkTreeView::SetStyle(DWORD flags, DWORD mask) const
{
	DWORD	aux;
	aux = ::GetWindowLong( hwnd,GWL_STYLE);
	aux&=~(mask);
	aux|= (flags&mask);
	::SetWindowLong( hwnd , GWL_STYLE, aux);
}

void
 TSonorkTreeView::HideToolTip() const
{
	HWND TThwnd;
	if((TThwnd = (HWND)::SendMessage(hwnd,TVM_GETTOOLTIPS,0,0)) != NULL )
		::SendMessage(TThwnd,TTM_POP,0,0);

}


TSonorkTreeItem*
 TSonorkTreeView::HitTest(
	  long 	scr_x
	, long 	scr_y
	, DWORD hit_flags
	, long*	client_y) const
{
	TV_HITTESTINFO hit_test;

	hit_test.pt.x = scr_x;
	hit_test.pt.y = scr_y;
	::ScreenToClient( hwnd, &hit_test.pt);

	if( client_y )
		*client_y=hit_test.pt.y;

	TreeView_HitTest(hwnd,&hit_test);
	if(hit_test.hItem != NULL && ( hit_test.flags & hit_flags) )
		return GetItem( hit_test.hItem );

	return NULL;
}

// Tries to fetch the deepest item with GetEventCount()<>0
// (searches from children towards root)
TSonorkTreeItem*
 TSonorkTreeView::GetFirstItemWithEvents(HTREEITEM pHitem) const
{
	TSonorkTreeItem*	VI;
	HTREEITEM		cHitem;
	for(	 cHitem = TreeView_GetChild(hwnd,pHitem)
		;cHitem != NULL
		;cHitem = TreeView_GetNextSibling(hwnd,cHitem))
	{
		VI = GetFirstItemWithEvents(cHitem);
		if( VI == NULL )
		{
			VI = GetItem( cHitem );
			if( VI == NULL )
				continue;
			if( VI->GetEventCount() == 0 )
				continue;
		}
		return VI;
	}
	return NULL;
}

// ----------------------------------------------------------------------------

void
 TSonorkTreeView::ExpandItemLevels(HTREEITEM hitem, DWORD expand_flags, int sub_levels) const
{
	TreeView_Expand( hwnd, hitem, expand_flags );
	if( sub_levels>0 )
	{
		for(hitem = TreeView_GetChild(hwnd, hitem)
			;hitem != NULL
			;hitem = TreeView_GetNextSibling(hwnd,hitem))
		{
			ExpandItemLevels( hitem, expand_flags , sub_levels-1 );
		}
	}
}


// ----------------------------------------------------------------------------

void
 TSonorkTreeView::SortChildren(HTREEITEM hitem, bool recursive) const
{
	TV_SORTCB 	sort;

	sort.hParent	=hitem;
	sort.lParam	=0;

	sort.lpfnCompare=SortCallback;
	TreeView_SortChildrenCB( hwnd , &sort , 0 );

	if( recursive )
	{
		for(	 hitem = TreeView_GetChild(hwnd, hitem)
			;hitem != NULL
			;hitem = TreeView_GetNextSibling(hwnd,hitem))
		{
			SortChildren( hitem , true);
		}
	}

}

// ----------------------------------------------------------------------------

int CALLBACK
 TSonorkTreeView::SortCallback(LPARAM P1,LPARAM P2,LPARAM)
{
	TSonorkTreeItem *I1,*I2;
	SKIN_ICON	dummy;
	I1=(TSonorkTreeItem*)P1;
	I2=(TSonorkTreeItem*)P2;

	if( I1->GetSortIndex() > I2->GetSortIndex() )
	{
		return -1;
	}
	else
	if( I1->GetSortIndex() < I2->GetSortIndex() )
	{
		return 1;
	}
	else
	{
		return stricmp(
			I1->GetLabel(false,dummy)
			,I2->GetLabel(false,dummy));
	}
}

// ----------------------------------------------------------------------------

void
 TSonorkTreeView::FocusItem(HTREEITEM hItem) const
{
	if( hItem != NULL && hItem != TVI_ROOT )
		TreeView_Select(hwnd , hItem ,TVGN_CARET );
}
void
 TSonorkTreeView::SelectItem(HTREEITEM hItem) const
{
	TreeView_SelectItem(hwnd,hItem);
}


// ----------------------------------------------------------------------------

void
 TSonorkTreeView::EnsureVisible(HTREEITEM hitem) const
{
	if( hitem != NULL && hitem != TVI_ROOT)
		TreeView_EnsureVisible(hwnd,hitem);
}

// ----------------------------------------------------------------------------

void
 TSonorkTreeView::DelItemChildren(HTREEITEM parent, BOOL mass_op) const
{
	HTREEITEM	cHitem,nHitem;
	for(	cHitem = TreeView_GetChild(hwnd,parent)
		; 	cHitem != NULL
		; 	cHitem = nHitem)
	{
		nHitem = TreeView_GetNextSibling(hwnd,cHitem);
		DelItemChildren( cHitem , mass_op );
		DelItem(cHitem , mass_op);
	}
}

TSonorkTreeItem*
 TSonorkTreeView::FindItem(SONORK_TREE_ITEM_TYPE type
	, DWORD search_id
	, HTREEITEM hItem) const
{
	TSonorkTreeItem* sI;
	TSonorkTreeItem* tI;
	for( 	sI=GetChild(hItem)
	;	sI!=NULL
	;	sI=GetNextSibling(sI) )
	{
		if( sI->Type() == type )
		{
			if( sI->SearchId() == search_id )
				return sI;

		}
		if((tI=FindItem(type,search_id,sI->hItem() ))!=NULL)
			return tI;

	}
	return NULL;
}

// ----------------------------------------------------------------------------

HTREEITEM
 TSonorkTreeView::AddItem(TSonorkTreeItem* PI
	, TSonorkTreeItem* VI
	, BOOL mass_op) const
{
	TV_INSERTSTRUCT 	tv_item;
	HTREEITEM		parent;
	assert( VI->hitem == NULL );
	parent = PI==NULL?TVI_ROOT:PI->hitem;

	VI->visual_state_flags  = VI->GetStateFlags();
	tv_item.hParent		= parent;
	tv_item.hInsertAfter	= TVI_LAST ;
	tv_item.item.mask 	= TVIF_TEXT | TVIF_IMAGE | TVIF_PARAM | TVIF_STATE ;
	tv_item.item.state	= VI->visual_state_flags ;
	tv_item.item.stateMask	= ITEM_STATE_FLAGS_SONORK_TREE_IS_AWARE_OF;
	tv_item.item.hItem  	= NULL;
	tv_item.item.iImage		= I_IMAGECALLBACK;
	tv_item.item.iSelectedImage	= I_IMAGECALLBACK;
	tv_item.item.pszText	= LPSTR_TEXTCALLBACK;
	tv_item.item.cChildren	= 0;
	tv_item.item.lParam	= (LPARAM)VI;

	VI->parent		= PI;
	VI->hitem 		= TreeView_InsertItem(hwnd,&tv_item);
	if( PI != NULL )
	{
		UpdateItemAttributes(
			PI
		,	VI->GetBoldCount()
		,	VI->GetEventCount()
		,	mass_op
			?(SONORK_TREE_VIEW_UPDATE_F_NO_PAINT)
			:(SONORK_TREE_VIEW_UPDATE_F_SORT
			| SONORK_TREE_VIEW_UPDATE_F_SORT_ITEM));
	}
	return VI->hitem;

}

// ----------------------------------------------------------------------------

void
 TSonorkTreeView::AfterMassOp( bool sort_root ) const
{
	HTREEITEM	hItem;
	if( sort_root )
	{
		SortChildren(TVI_ROOT,true);
	}
	else
	{
		for(	  hItem = TreeView_GetChild(hwnd,TVI_ROOT)
			; hItem != NULL
			; hItem = TreeView_GetNextSibling(hwnd,hItem))
			SortChildren(hItem,true);
	}
	MassRepaint(TVI_ROOT);
	InvalidateRect( NULL, false);
}

// ----------------------------------------------------------------------------

void
 TSonorkTreeView::MassRepaint(HTREEITEM hItem) const
{
	TSonorkTreeItem* VI;
	TV_ITEM		tvi;
	DWORD		item_state_flags;
	tvi.mask = TVIF_HANDLE|TVIF_PARAM;

	for(	tvi.hItem = TreeView_GetChild(hwnd,hItem)
	;	tvi.hItem != NULL
	; 	tvi.hItem = TreeView_GetNextSibling(hwnd,tvi.hItem))
	{
		if(!TreeView_GetItem(hwnd,&tvi))
			break;
		VI=(TSonorkTreeItem*)tvi.lParam;
		assert( VI != NULL );
		item_state_flags=VI->GetStateFlags();
		if((VI->visual_state_flags ^ item_state_flags)&(ITEM_STATE_FLAGS_SONORK_TREE_IS_AWARE_OF) )
			DoRepaint(VI
				, &item_state_flags
				, false);
		MassRepaint( tvi.hItem );
	}
}

// ----------------------------------------------------------------------------

void
 TSonorkTreeView::DelItem(TSonorkTreeItem* VI , BOOL mass_op) const
{
	if( VI == NULL )return;

	UpdateItemAttributes(
		 VI->parent
	,	-VI->GetBoldCount()
	,	-VI->GetEventCount()
	,	 mass_op?
		 (SONORK_TREE_VIEW_UPDATE_F_NO_PAINT)
		:(SONORK_TREE_VIEW_UPDATE_F_SORT)
		 );
	TreeView_DeleteItem( hwnd , VI->hitem );
}

// ----------------------------------------------------------------------------

void
 TSonorkTreeView::DelItem( HTREEITEM hItem , BOOL mass_op) const
{
	DelItem(GetItem(hItem),mass_op);
}

// ----------------------------------------------------------------------------

void
 TSonorkTreeView::UpdateItemAttributes(
			  TSonorkTreeItem*	VI
			, int 			bold_delta
			, int 			event_delta
			, DWORD 		update_flags ) const
{
	update_flags&=0xff;
	if(bold_delta || event_delta )
	{
		update_flags|=	 SONORK_TREE_VIEW_UPDATE_iF_AUTO_PAINT
				|SONORK_TREE_VIEW_UPDATE_iF_PROPAGATE;
	}
	if( update_flags & SONORK_TREE_VIEW_UPDATE_F_NO_PAINT )
		update_flags&=~(SONORK_TREE_VIEW_UPDATE_F_FORCE_PAINT|SONORK_TREE_VIEW_UPDATE_iF_AUTO_PAINT);

	while( VI != NULL )
	{
		VI->IncBoldCount(bold_delta);
		VI->IncEventCount(event_delta);

		if( update_flags & (SONORK_TREE_VIEW_UPDATE_F_FORCE_PAINT|SONORK_TREE_VIEW_UPDATE_iF_AUTO_PAINT) )
		{
			Repaint( VI );
		}

		if( update_flags & SONORK_TREE_VIEW_UPDATE_F_SORT_ITEM )
		{
			// F_SORT_ITEM: Sort item itself
			SortChildren( VI->hitem , false );
			update_flags &=~ SONORK_TREE_VIEW_UPDATE_F_SORT_ITEM;
		}
		VI = VI->parent;
		if( VI==NULL)
			return;

		// F_SORT: Sort items (starting at parent)
		if( update_flags & SONORK_TREE_VIEW_UPDATE_F_SORT )
		{
			// Remove SORT_PARENT: We're sorting the parent here
			SortChildren( VI->hitem , false );
		}
		if( !(update_flags&SONORK_TREE_VIEW_UPDATE_iF_PROPAGATE ) )
			return;
	}

}

// ----------------------------------------------------------------------------

void
 TSonorkTreeView::Repaint(TSonorkTreeItem* VI) const
{
	DWORD	*new_state_flags;
	DWORD 	item_state_flags;
	if( VI->hitem == NULL || VI->hitem == TVI_ROOT )
		return;
	item_state_flags = VI->GetStateFlags();
	if( (VI->visual_state_flags ^ item_state_flags)&(ITEM_STATE_FLAGS_SONORK_TREE_IS_AWARE_OF))
	{
		// flags have changed
		new_state_flags=&item_state_flags;
	}
	else
		new_state_flags=NULL;
	DoRepaint( VI , new_state_flags, true );
}

void
 TSonorkTreeView::DoRepaint( TSonorkTreeItem* VI
	, DWORD* new_state_flags
	, BOOL	 invalidate_rect) const
{
	TV_ITEM	tv_item;
	RECT 	rect;
	if( new_state_flags)
	{
		tv_item.hItem		= VI->hitem;
		tv_item.mask 		= TVIF_STATE ;
		VI->visual_state_flags	=
		tv_item.state		= *new_state_flags ;
		tv_item.stateMask	= ITEM_STATE_FLAGS_SONORK_TREE_IS_AWARE_OF ;
		tv_item.cChildren	= 0;
		TreeView_SetItem( hwnd , &tv_item );

	}
	if( invalidate_rect )
	{
		if(TreeView_GetItemRect(hwnd,VI->hitem,&rect,false))
			InvalidateRect(&rect,true);
	}
}


// ----------------------------------------------------------------------------

LRESULT
 TSonorkTreeItem::OnCustomDraw(NMTVCUSTOMDRAW* CD)
{
	TSonorkTreeItem*VI;
	if(CD->nmcd.dwDrawStage == CDDS_PREPAINT )
	{
		return CDRF_NOTIFYITEMDRAW;
	}

	if( CD->nmcd.dwDrawStage == CDDS_ITEMPREPAINT )
	{
		VI=(TSonorkTreeItem*)CD->nmcd.lItemlParam;
		return VI->CustomDraw_Prepaint(CD);
	}
	return CDRF_DODEFAULT;
}

// ----------------------------------------------------------------------------

LRESULT
 TSonorkTreeItem::OnGetDispInfo(TV_DISPINFO* DI)
{
	TSonorkTreeItem*VI;
	VI=(TSonorkTreeItem*)DI->item.lParam;

	assert( VI != NULL );

	DI->item.mask|=TVIF_SELECTEDIMAGE
			|TVIF_IMAGE
			|TVIF_TEXT;
	DI->item.iSelectedImage=
	DI->item.iImage=
		VI->GetDispInfoInto(
			 DI->item.state&(TVIS_EXPANDED|TVIS_EXPANDPARTIAL)
			,DI->item.pszText
			,DI->item.cchTextMax);
	return 0;
}

// ----------------------------------------------------------------------------

TSonorkTreeItem::TSonorkTreeItem(SONORK_TREE_ITEM_TYPE nt)
{
	type		= nt;
	hitem		= NULL;
	parent		= NULL;
}


// ----------------------------------------------------------------------------
// GetDispInfoInto
// Gets the Disposition and View Information into the buffer provided.

SKIN_ICON
 TSonorkTreeItem::GetDispInfoInto(BOOL expanded,char*buffer,int buffer_size) const
{
	SKIN_ICON    	icon;
	int		e_count;

	e_count = GetEventValue();
	if(!(e_count & SONORK_TREE_ITEM_VALUE_F_HIDE_COUNT) )
	if( (e_count &=SONORK_TREE_ITEM_VALUE_F_COUNT) != 0)
	{
		wsprintf(buffer,"%s (%u)"
			, GetLabel(expanded,icon)
			, e_count );
		return icon;
	}
	lstrcpyn(buffer,GetLabel(expanded,icon),buffer_size);
	return icon;
}

LRESULT
 TSonorkTreeItem::CustomDraw_Prepaint(NMTVCUSTOMDRAW* CD ) const
{
	::FillRect( CD->nmcd.hdc
	, &CD->nmcd.rc
	, sonork_skin.Brush(SKIN_BRUSH_MAIN_VIEW));
	if( CD->nmcd.uItemState&(CDIS_FOCUS|CDIS_SELECTED) )
	{
		CD->clrText
			= sonork_skin.Color(SKIN_COLOR_MSG_FOCUS,SKIN_CI_FG);
		CD->clrTextBk
			= sonork_skin.Color(SKIN_COLOR_MSG_FOCUS,SKIN_CI_BG);
	}
	else
	if( GetEventCount() )
		CD->clrText = sonork_skin.Color(SKIN_COLOR_MAIN_EXT
				,SKIN_CI_MAIN_EVENT);
	return CDRF_DODEFAULT;

}

