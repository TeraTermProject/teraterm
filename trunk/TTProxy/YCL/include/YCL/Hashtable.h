/*
 * $Id: Hashtable.h,v 1.4 2007-08-18 08:52:18 maya Exp $
 */

#ifndef _YCL_HASHTABLE_H_
#define _YCL_HASHTABLE_H_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include <YCL/common.h>

#include <YCL/HASHCODE.h>
#include <YCL/ValueCtrl.h>
#include <YCL/Enumeration.h>
#include <YCL/Pointer.h>

namespace yebisuya {

// ハッシュテーブルを管理するクラス。
template<class TYPE_KEY, class TYPE_VALUE, class KEYCTRL = ValueCtrl<TYPE_KEY>, class VALCTRL = ValueCtrl<TYPE_VALUE> >
class Hashtable {
	// TYPE_KEY, TYPE_VALUEにとれる型:
	// ・デフォルトコンストラクタを持つ
	// ・コピーコンストラクタを持つ
	// ・==演算子を持つ
	// ・=演算子を持つ(この条件のため参照は指定できない)
	// KEYCTRL、VALCTRLにデフォルトのものを使用する場合は
	// ・デフォルトコンストラクタで生成されたインスタンスとNULLを比較すると真になる
	// ・NULLインスタンスとNULLを比較すると真になる
	// ・TYPE_KEYはハッシュ値を生成するhashCodeメソッドを持つ
	// が条件に加わる。
public:
	typedef Enumeration<TYPE_KEY> KeyEnumeration;
	typedef Enumeration<TYPE_VALUE> ElementEnumeration;
private:
	// コピーコンストラクタは使用禁止
	Hashtable(Hashtable&);
	// 代入演算子は使用禁止
	void operator=(Hashtable&);

	// ハッシュテーブルのエントリ。
	struct Entry {
		TYPE_KEY key;
		TYPE_VALUE value;
		Entry() {
			KEYCTRL::initialize(key);
			VALCTRL::initialize(value);
		}
	};
	// ハッシュテーブルのエントリを列挙するためのクラス。
	class EnumEntries {
	private:
		const Hashtable& table;
		mutable int index;
		void skip()const {
			while (index < table.backetSize && VALCTRL::isEmpty(table.backet[index].value))
				index++;
		}
	public:
		EnumEntries(const Hashtable& table)
		:table(table), index(0) {
			skip();
		}
		bool hasMoreEntries()const {
			return index < table.backetSize;
		}
		const Entry& nextEntry()const {
			const Entry& entry = table.backet[index++];
			skip();
			return entry;
		}
	};
	friend class EnumEntries;
	// ハッシュテーブルのキーを列挙するためのクラス。
	class EnumKeys : public KeyEnumeration {
	private:
		EnumEntries entries;
	public:
		EnumKeys(const Hashtable& table)
		:entries(table) {
		}
		virtual bool hasMoreElements()const {
			return entries.hasMoreEntries();
		}
		virtual TYPE_KEY nextElement()const {
			return entries.nextEntry().key;
		}
	};
	// ハッシュテーブルの値を列挙するためのクラス。
	class EnumValues : public ElementEnumeration {
	private:
		EnumEntries entries;
	public:
		EnumValues(const Hashtable& table)
		:entries(table) {
		}
		virtual bool hasMoreElements()const {
			return entries.hasMoreEntries();
		}
		virtual TYPE_VALUE nextElement()const {
			return entries.nextEntry().value;
		}
	};

	// エントリの配列。
	Entry* backet;
	// エントリの配列のサイズ
	int backetSize;
	// 有効なエントリの数
	int count;
	enum {
		// エントリの配列を大きくするときに使用するサイズ
		BOUNDARY = 8,
	};
	// keyと等しいキーを持つエントリのインデックスを返す。
	// 引数:
	//	key	検索するキー
	// 返値:
	//	keyと等しいキーを持つか、まだ設定されていないエントリのインデックス。
	//	全てのエントリが設定済みでkeyと等しいものがなければ-1を返す。
	int find(const TYPE_KEY& key)const {
		int found = -1;
		int h = HASHCODE(key);
		for (int i = 0; i < backetSize; i++) {
			int index = ((unsigned) h + i) % backetSize;
			const TYPE_KEY& bkey = backet[index].key;
			if (KEYCTRL::isEmpty(bkey) || bkey == key) {
				found = index;
				break;
			}
		}
		return found;
	}
	// エントリの配列を指定のサイズに変え、もう一度テーブルを作り直す。
	// 引数:
	//	newSize	新しい配列のサイズ。
	//			有効なエントリの数よりも少なくしてはいけない。
	void rehash(int newSize) {
		Entry* oldBacket = backet;
		int oldSize = backetSize;
		backet = new Entry[newSize];
		backetSize = newSize;
		for (int i = 0; i < oldSize; i++) {
			if (!VALCTRL::isEmpty(oldBacket[i].value)) {
				backet[find(oldBacket[i].key)] = oldBacket[i];
			}
		}
		delete[] oldBacket;
	}
public:
	// デフォルトコンストラクタ。
	// 空のハッシュテーブルを作る。
	Hashtable()
	:backet(NULL), backetSize(0), count(0) {
	}
	// エントリの配列の初期サイズを指定するコンストラクタ。
	// 引数:
	//	backetSize	エントリの配列の大きさ。
	Hashtable(int backetSize)
	:backet(new Entry[backetSize]), backetSize(backetSize), count(0) {
	}
	// デストラクタ。
	// エントリの配列を削除する。
	~Hashtable() {
		delete[] backet;
	}
	// 指定のキーに対応する値を取得する。
	// 引数:
	//	key	検索するキー
	// 返値:
	//	キーに対応する値。見つからなければNULLと等しい値を返す。
	TYPE_VALUE get(const TYPE_KEY& key)const {
		TYPE_VALUE value;
		VALCTRL::initialize(value);
		int index = find(key);
		if (index != -1)
			value = backet[index].value;
		return value;
	}
	// 指定のキーに対応する値を設定する。
	// 引数:
	//	key	設定するキー
	//	value 設定する値
	// 返値:
	//	前にキーに設定されていた値。未設定だった場合はNULLと等しい値を返す。
	TYPE_VALUE put(const TYPE_KEY& key, const TYPE_VALUE& value) {
		int index = find(key);
		if (index == -1) {
			rehash(backetSize + BOUNDARY);
			index = find(key);
		}
		TYPE_VALUE old = backet[index].value;
		if (VALCTRL::isEmpty(old)) {
			count++;
			backet[index].key = key;
		}
		backet[index].value = value;
		return old;
	}
	// 指定のキーに対応する値を削除する。
	// 引数:
	//	key	削除するキー
	// 返値:
	//	キーに設定されていた値。未設定だった場合はNULLと等しい値を返す。
	TYPE_VALUE remove(const TYPE_KEY& key) {
		TYPE_VALUE old;
		VALCTRL::initialize(old);
		int index = find(key);
		if (index != -1) {
			old = backet[index].value;
			if (!VALCTRL::isEmpty(old)) {
				count--;
				VALCTRL::initialize(backet[index].value);
				if (backetSize - count > BOUNDARY)
					rehash(count);
			}
		}
		return old;
	}
	// キーを列挙するためのインスタンスを生成する。
	// 使用後はdeleteを忘れないように注意。
	// 返値:
	//	キーを列挙するためのインスタンス。
	Pointer<KeyEnumeration> keys()const {
		return new EnumKeys(*this);
	}
	// 値を列挙するためのインスタンスを生成する。
	// 使用後はdeleteを忘れないように注意。
	// 返値:
	//	値を列挙するためのインスタンス。
	Pointer<ElementEnumeration> elements()const {
		return new EnumValues(*this);
	}
	// 有効な値を持つキーの数を取得する。
	// 返値:
	//	有効な値を持つキーの数。
	int size()const {
		return count;
	}
	// 指定のキーが有効な値を持っているかどうかを判定する。
	// 引数:
	//	判定するキー。
	// 返値:
	//	有効な値を持っていれば真。
	bool contains(const TYPE_KEY& key)const {
		int index = find(key);
		return index != -1 && !VALCTRL::isEmpty(backet[index].value);
	}
	// 有効な値を一つも持っていないかどうかを判定する。
	// 返値:
	//	有効な値を一つも持っていなければ真。
	bool isEmpty()const {
		return count == 0;
	}
	// 有効な値を一つも持っていない状態に戻す。
	void empty() {
		delete[] backet;
		backet = NULL;
		backetSize = 0;
		count = 0;
	}
};

}

#endif//_YCL_HASHTABLE_H_
