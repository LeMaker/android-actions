package com.actions.fragment;

import com.actions.model.TestContent;
import android.app.Activity;
import android.app.ListFragment;
import android.os.Bundle;
import android.util.Log;
import android.view.View;
import android.widget.ArrayAdapter;
import android.widget.ListView;
/**
 * Description:
 * this is for sleceting testing item
 *********************************************   
 *ActionsCode(author:jiangjinzhang, new_code)
 * @version 1.0
 */

public class HeaderListFragment extends ListFragment
{
	private static final String TAG = "TestListFragment";
	private Callbacks mCallbacks;
	private int cur_pos = 0;
	public interface Callbacks
	{
		public void onItemSelected(Integer id);
	}

	@Override
	public void onCreate(Bundle savedInstanceState)
	{
		super.onCreate(savedInstanceState);
		setListAdapter(new ArrayAdapter<TestContent.TestItem>(getActivity(),
				android.R.layout.simple_list_item_activated_1,
				android.R.id.text1, TestContent.ITEMS));	
	}
	
		
	@Override
	public void onAttach(Activity activity)
	{
		super.onAttach(activity);
		
		if (!(activity instanceof Callbacks))
		{
			throw new IllegalStateException(
				"ListFragment");
		}
		
		mCallbacks = (Callbacks)activity;
	}

	@Override
	public void onDetach()
	{
		super.onDetach();
		
		mCallbacks = null;
	}


	@Override
	public void onListItemClick(ListView listView
		, View view, int position, long id)
	{
		super.onListItemClick(listView, view, position, id);
		//highlight background
		listView.setChoiceMode(ListView.CHOICE_MODE_SINGLE);
		 cur_pos = position;
		 Log.d(TAG, "cur_pos="+cur_pos);
		System.out.println("position="+position+"  id= "+id);
		mCallbacks.onItemSelected(TestContent.ITEMS.get(position).id);
	}

	public void setActivateOnItemClick(boolean activateOnItemClick)
	{
		getListView().setChoiceMode(
				activateOnItemClick ? ListView.CHOICE_MODE_SINGLE
						: ListView.CHOICE_MODE_NONE);
	}
}
