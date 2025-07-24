#include <iostream>
#include <chrono>
#include <string>
#include <sstream>
#include <utils/dispatcher_base.hpp>

struct person_one
{
	char name[30];
	int age;
	bool isAdult;
};


class sample_one_disp : public afu::dispatcher_base
{
	person_one a;
	std::shared_ptr<afu::subscriber> m_subs;

public:

	sample_one_disp(std::shared_ptr<afu::subscriber> _s) :
		m_subs(_s)
	{
		std::string na = "liran";
		std::strcpy(a.name, na.c_str());
		a.age = 8;
		a.isAdult = true;
	}

	sample_one_disp(const afu::subscriber& _s):
		m_subs(std::make_shared<afu::subscriber>(_s))
	{
		std::string na = "liran";
		std::strcpy(a.name, na.c_str());
		a.age = 8;
		a.isAdult = true;
	}

	virtual void init() override
	{
		subscribe(m_subs, [&](const std::shared_ptr<afu::subscription_data>& data)
			{
				auto d = data->read<person_one>();
				std::stringstream ss;
				ss << "got new person " << d.name << "with age " << d.age << "and he is " << (d.isAdult ? "Adult\n" : "Kid\n");
				std::cout << ss.str();
			});
	}

};

int main()
{

	std::vector<char*> a11 = { "liran", "schwartz", "YESS" };
	std::vector<int> a1 = { 12, 33,44 };
	std::vector<bool> a2 = { false, true, true};

	std::cout << "Start\n";
	std::shared_ptr<afu::subscriber> sub = std::make_shared<afu::subscriber>( sizeof(person_one));

	sample_one_disp a(sub);
	a.init();
	a.start();
	std::chrono::milliseconds _duration(2000);
	int ind = 0;
	while (true)
	{
		person_one p = {"aa", a1[ind], a2[ind]};
		ind = (++ind) % 3;
		sub->write(p);
		std::this_thread::sleep_for(_duration);
	}
	a.stop();
	std::cout  << "bye\n";
	return 0;
}