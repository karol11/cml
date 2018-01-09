package com.github.karol11.cml;

import java.io.FileReader;
import java.io.FileWriter;
import java.io.IOException;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

class Phone {
	static final int HOME = 1;
	String number;
	int type;

	Phone(){}
	
	public Phone(CmlStaxReader r) throws IOException {
		for (;;) {
			switch (r.next()) {
			case CmlStaxReader.R_LONG:
				if (r.getField().equals("type"))
					type = (int) r.getLongVal();
				else
					r.error("expected type field");
				break;
			case CmlStaxReader.R_STRING:
				if (r.getField().equals("number"))
					number = r.getStrVal();
				else
					r.error("expected number field");
				break;
			case CmlStaxReader.R_STRUCT_END:
				return;
			default:
				r.error("unexpected type");
			}
		}
	}
}

class Person {
	String name;
	String email;
	int id;
	List<Phone> phones = new ArrayList<>();

	Person(){}
	
	public Person(CmlStaxReader r) throws IOException {
		for (;;) {
			switch (r.next()) {
			case CmlStaxReader.R_STRUCT_END:
				return;
			case CmlStaxReader.R_LONG:
				if (!r.getField().equals("id"))
					r.error("expected field id");
				id = (int) r.getLongVal();
				break;
			case CmlStaxReader.R_STRING:
				if (r.getField().equals("name"))
					name = r.getStrVal();
				else if (r.getField().equals("email"))
					email = r.getStrVal();
				else
					r.error("expected field email or name");
				break;
			case CmlStaxReader.R_ARRAY_START:
				if (!r.getField().equals("phones"))
					r.error("expected field phones");
				for (int i = 0; (i = r.next()) != CmlStaxReader.R_ARRAY_END;) {
					if (i != CmlStaxReader.R_STRUCT_START || !r.getType().equals("Phone"))
						r.error("expected struct Phone");
					phones.add(new Phone(r));
				}
				break;
			default:
				r.error("unexpected field type");
			}
		}
	}
}

class AddressBook {
	List<Person> people = new ArrayList<>();

	public AddressBook() {}

	public AddressBook(CmlStaxReader r) throws IOException {
		if (r.next() != CmlStaxReader.R_ARRAY_START || !r.getField().equals("people"))
			r.error("expected array people");
		for (int i = 0; (i = r.next()) != CmlStaxReader.R_ARRAY_END;) {
			if (i != CmlStaxReader.R_STRUCT_START || !r.getType().equals("Person"))
				r.error("expected struct Person");
			people.add(new Person(r));
		}
		if (r.next() != CmlStaxReader.R_STRUCT_END)
			r.error("extra fields in address book");
	}
}

public class Test {
	static public void main(String[] args) throws IOException {
		Dom d = new Dom();
		Dom.StructType person = d.getOrCreateStruct("Person", "name", "email", "phones", "id");
		Dom.StructType phone = d.getOrCreateStruct("Phone", "number", "type");
		d.root = d.getOrCreateStruct("AddressBook", "people").make()
			.set("people", Arrays.asList(
				person.make()
					.set("name", "John Doe")
					.set("id", 1234)
					.set("email", "jdoe@example.com")
					.set("phones", Arrays.asList(
						phone.make().set("number", "555-4321").set("type", Phone.HOME)))));
		try(FileWriter f = new FileWriter("pb.cml")) {
			CmlDomWriter.write(d, f);			
		}
		try(FileReader f = new FileReader("pb.cml")) {
			CmlStaxReader r = new CmlStaxReader(f);
			if (r.next() != CmlStaxReader.R_STRUCT_START || !r.getType().equals("AddressBook"))
				r.error("expected struct AddressBook");
			AddressBook p = new AddressBook(r);
			System.out.println(p.people.get(0).name);
		}
	}
}
