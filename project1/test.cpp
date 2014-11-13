#include <iostream>
#include <vector>

struct list
{
	int value;
	list* next;
};

class singleton
{
public:
	static singleton *self;
	static singleton * get()
	{
		return self;
	}
};
singleton * singleton::self = new singleton();

list* push_back(list* l, int v)
{
	list* new_list = new list;
	new_list->value = v;
	new_list->next = (list*)0;
	l->next = new_list;
	return new_list;
}

void print_list(list* iter)
{
	std::cout << "print" << std::endl;
	while(iter != (list*)0)
	{
		std::cout << iter->value << "\n";
		iter = iter->next;
	}
}
list* reverse(list* iter_f)
{
	list *tmp_iter;
	list *pre = (list*) 0;
	list *iter = iter_f;
	while(iter)
	{
		tmp_iter = iter; //複製一份現在的
		iter = iter->next; //現在變成下一個
		tmp_iter->next = pre;//現在的指向改成現在
		pre = tmp_iter;//
		//if(iter == iter_f) // 防環
		//	return tmp_iter;
	}
	return tmp_iter;
}
// a c f b a c f b ...
// 1,2,3
int t(int n, int add, int &count)
{
	int c = 1;
	int base = 0;
	for (int i=1; i<=n; i++)
	{	
		if((i+1)%2 == 0 && (i+1)%3 == 0)
			base++;
		if(i%2==0 || i%3==0)
			c++;
		c += base;
	}
	count = c;
	return 0;
}
list* reverse2(list* iter_o)
{
	list* iter = iter_o;
	list* iter_pre = (list*)0;
	list* tmp_iter;
	while(iter)
	{
		tmp_iter = iter;
		iter = iter->next;
		tmp_iter->next = iter_pre;
		iter_pre = tmp_iter;
	}
	return tmp_iter;
}
list* delete_list2(list* iter_o, int v)
{
	list* iter = iter_o;
	list* iter_pre = (list*)0;
	while (iter)
	{
		if(iter->value == v)
		{
			if(iter_pre)
				iter_pre->next = iter->next;
			else
				iter_o = iter->next;
			delete iter;
		}
		else
			iter_pre = iter;
		iter = iter->next;
	}
	return iter_o;
}
int find_c(std::vector<int>& list)
{
	int count = 1;
	int cal = list[0];
	int max_count = 1;
	int max_cal = list[0];
	
	for(auto i=1; i<list.size(); i++)
	{
		if(list[i-1]+1 == list[i])
		{
			count++;
			cal += list[i];
			if(count > max_count)
			{
				max_count = count;
				max_cal = cal;
			}
		}
		else
		{
			count = 1;
			cal = list[i];
		}
	}
	std::cout << "max count " << max_count << " max_cal " << max_cal << std::endl;
	return 0;
}

//  cga
//atcgatcg

// 1.如果沒有環的話，兩相交link list的尾巴會相同
// 2.如果有環的話，兩相交link，一快一慢，遲早會遇到相等

int t2(int n, int add, int &count)
{
	n = n-add;
	
	if(n==0)
	{
		count++;
		
		//if(add != 0) std::cout << "|" << add << " ";
		return add;
	}
	
	if(n < 0) return 0;
	
	//if(add <= 1)
	{
		int v = t2(n, 1, count);
		//if(v!=0 && add !=0) std::cout << add << " ";
	}
		
	//if(add <= 2)
	{
		int v = t2(n, 2, count);
		//if(v!=0 && add !=0) std::cout << add << " ";
	}
	//if(add <= 3)
	{
		int v = t2(n, 3, count);
		//if(v!=0 && add !=0) std::cout << add << " ";
	}	
	return add;
}

list* delete_list(list* iter_o, int v)
{
	list* iter = iter_o;
	list* iter_pre = (list*)0;
	while(iter != (list*)0)
	{
		if(iter->value == v)
		{
			if(iter_pre)// 前面有
				iter_pre->next = iter->next;
			else // 前面沒有
				iter_o = iter->next;
			delete iter;
		}
		else
			iter_pre = iter;
		iter = iter->next;
	}
	return iter_o;
}

bool is_same(list* a_iter, list* b_iter)
{
	list* iter = a_iter;
	while(iter)
	{
		if(iter == b_iter)
			return true;
		iter = iter->next;
		if(iter == a_iter) // loop
			return false;
	}
	return false;
}
void qsort(std::vector<int>::iterator iter_a, std::vector<int>::iterator iter_b)
{
	
	if (iter_b - iter_a < 2 )
		return;
	int q = *(iter_a + (iter_b - iter_a)/2);
	
	//std::cout << "min " << *iter_a <<  " max " << *(iter_b-1) <<  " q "<< q << " "<< iter_b - iter_a<< std::endl;
	//for( auto iter = iter_a ;iter != iter_b; iter++ )
	//	std::cout << *iter << " ";
	//std::cout << "\n";
	
	auto iter_min = iter_a;
	auto iter_max = iter_b-1;
	while(true)
	{
		while(iter_min != iter_b && *iter_min < q)
			iter_min++;
		while(iter_max != iter_a && *iter_max > q)
			iter_max--;
		if(iter_min >= iter_max)
			break;
		
		std::swap(*iter_min++, *iter_max--);
	}
	//std::cout << "C*iter_min " << *iter_a << " *iter_min" << *(iter_min) << " *iter_max" << *(iter_max) << std::endl;
	
	qsort(iter_a, iter_min);
	qsort(iter_max+1, iter_b);
}

//#include "test2.hpp"

static int gg = 9;
extern int gg;


class cell_phone
{
public:
	int price;
	std::string brand;
	cell_phone()
		:price(100), brand("HTC")
	{}
	friend std::ostream& operator<<(std::ostream &out, const cell_phone &c)
	{
		out << c.price << " " << c.brand << std::endl;
		return out;
	}
	~cell_phone()
	{
		std::cout << "Bye~" << std::endl;
	}
};
class smart_phone
	:public cell_phone
{
public:
	smart_phone()
	{
		
	}
	friend std::ostream& operator<<(std::ostream &out, const smart_phone &s)
	{
		out << s.price << " none name" << std::endl;
		return out;
	}
};

struct B
{
	typedef B TYPE; 
	void p()
	{
		auto p = (this);
		//p->print();
		this->print();
	}
	void print()
	{
		std::cout << "B" << std::endl;
	}
};
struct D
	:B
{
	typedef D TYPE;
	void print()
	{
		std::cout << "D" << std::endl;
	}
};

template<class T>
void print(T &t)
{
	t.print();
}

// void (*aa) (int, int)
int main()
{
	int kk = 0;
	auto gg = reinterpret_cast<std::vector<int>*> (&kk);
	//char *aa[2] = {"aaa"};
	//cell_phone cc;
	//smart_phone sp;
	//out(cc);
	//out(sp);
	
	int a = 1 << 1 + 2*2 >> 2;
	std::cout << a << std::endl;
	//B b;
	D d;
	
	B &b1 = d;
	//b1.p();
	//b1.print();
	
	uint32_t b = 0;
	uint32_t c = 165;
	
	b = (~c) >> 4;

	
	std::cout << b << std::endl;
	
	//print(b1);
	
	
	//std::cout << cc << std::endl;
	//std::cout << sp << std::endl;
	
	//int gg[1];
	//int *kk = gg;
	//unsigned int a = 0XA5;
	//unsigned int b = ~a>>4;
	//aaa::kk++;
	//std::cout <<gg << " " << aaa::kk << std::endl;


return 0;
/*
	std::vector<int> array {6, 2, 7, 5, 3, 9, 3};
	qsort(array.begin(), array.end());
	for(int i : array)
		std::cout << i << " ";
	std::cout << "\n";
	
	return 0;
*/
	for (int i=1; i<20;i++)
	{
		int num = 0;
		
		t(i,0,num);
		std::cout <<"           i " << i << ";  num:" << num << std::endl;
	}
	
	
	for (int i=1; i<20;i++)
	{
		int num = 0;
		
		t2(i,0,num);
		std::cout <<"           i " << i << ";  num:" << num << std::endl;
	}
	
	
	
	//return 0;

	list* first_list = new list;
	first_list->value = 0;
	first_list->next = (list*)0;
	list *iter = first_list;
	
	for(int i=1; i<5; i++)
	{
		iter = push_back(iter, i);
		std::cout << i << std::endl;
	}
	print_list(first_list);
	iter = reverse2(first_list);
	print_list(first_list);
	print_list(iter);
	iter = delete_list2(iter, 4);
	//iter = delete_list(iter, 2);
	print_list(iter);
	
	std::vector<int> ll {6, 2, 7, 1, 2, 3, 4, 1, 2, 4};
	find_c(ll);
	
	
	
	
	
	
	
	
	return 0;
}