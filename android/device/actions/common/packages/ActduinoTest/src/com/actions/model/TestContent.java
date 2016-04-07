package com.actions.model;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

/**
 * Description:
 * this is for the constructor of  HeaderListFragment
 *********************************************   
 *ActionsCode(author:jiangjinzhang, new_code)
 * @version 1.0
 */

public class TestContent
{
	public static class TestItem
	{
		public Integer id;
		public String title;
		public String desc;

		public TestItem(Integer id, String title)
		{
			this.id = id;
			this.title = title;
		}

		@Override
		public String toString()
		{
			return title;
		}
	}
	
	public static List<TestItem> ITEMS = new ArrayList<TestItem>();
	
	public static Map<Integer, TestItem> ITEM_MAP 
		= new HashMap<Integer, TestItem>();

	static
	{
	
		addItem(new TestItem(1, "GPIO"));
		addItem(new TestItem(2, "UART"));
		addItem(new TestItem(3, "I2C"));
		addItem(new TestItem(4, "SPI"));
		addItem(new TestItem(5, "PWM&&ADC"));
	}

	private static void addItem(TestItem testitem)
	{
		ITEMS.add(testitem);
		ITEM_MAP.put(testitem.id, testitem);
	}
}
