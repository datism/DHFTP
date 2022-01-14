#include <iostream>
#include <vector>
#include <string>
using namespace std;

string getXepLoai(float diem);

class NhanVien {
public:
	string maNhanVien;
	string hoVaTen;
	int namSinh;
	float heSoLuong;
	string chucDanh;
	int tongLuong;
	float cdp;

	NhanVien(string iMaNhanVien, string iHoVaTen, int iNamSinh, float iheSoLuong, string ichucDanh, int luongToiThieu) {
		this->maNhanVien = iMaNhanVien;
		this->hoVaTen = iHoVaTen;
		this->namSinh = iNamSinh;
		this->heSoLuong = iheSoLuong;
		this->chucDanh = ichucDanh;
		this->tongLuong = luongToiThieu * heSoLuong + getPccv(ichucDanh);
		this->cdp = tongLuong * 0.01;

	}

	int getPccv(string chucDanh) {
		if (chucDanh == "GVCC")
			return  2000;
		if (chucDanh == "GVC")
			return  1500;
		if (chucDanh == "GV")
			return  1000;
		if (chucDanh == "CBKT")
			return 750;
		if (chucDanh == "CBHC")
			return 500;

		return 0;
	}
};

class PhongBan {
public:
	string ten;
	string diaChi;
	int luongToiThieu;
	int soNhanVien;
	float tongLuong;
	float tongCD;

	vector<NhanVien> nhanVienList;

	PhongBan(string iTen, string iDiaChi, int iLuongth, int isoNhanVien) {
		this->ten = iTen;
		this->diaChi = iDiaChi;
		this->luongToiThieu = iLuongth;
		this->soNhanVien = isoNhanVien;
	}


	void getTongLuong() {

		for (NhanVien nv : nhanVienList) {
			this->tongLuong += nv.tongLuong;
		}

	}

	void getCd() {

		for (NhanVien nv : nhanVienList) {
			this->tongCD += nv.cdp;
		}

	}


	friend ostream& operator<<(ostream& out, const PhongBan pb) {
	

		out << "Ten phong ban: " << pb.ten << '\n';
		out << "Dia chi: " << pb.diaChi << '\n';
		out << "Luong toi thieu:" << pb.luongToiThieu << '\n';
		out << "Tong luong :" << pb.tongLuong << '\n';
		out << "Tong cd phi :" << pb.tongCD << '\n';
		out << "Bang luong nhan vien:\n";
		out << "MaNV\tHotenNV\tChucDanh\tPCCV\tHeSoL\tLuong\tCDP\n";

		for (NhanVien nv : pb.nhanVienList) {
			out << nv.maNhanVien << '\t' << nv.hoVaTen << '\t' << nv.chucDanh << "\t\t" << nv.getPccv(nv.chucDanh) << '\t' << nv.heSoLuong << '\t' << nv.tongLuong << '\t' << nv.cdp << '\n';
		}

		return out;
	}
};

vector<PhongBan> gPhongBanList;

void nhapPhongBan();
void inThongTin();

int main() {

	cout << "0. Thoat" << '\n';
	cout << "1. Nhap cac phong ban" << '\n';
	cout << "2. In ket qua ds phong ban va Nhan vien" << '\n';

	int choice;
	while (1) {
		cout << "\nNhap lua chon: ";
		cin >> choice;
		cin.ignore(256, '\n');

		switch (choice) {
		case 0: return 0;
		case 1:
			nhapPhongBan();
			break;
		case 2:
			inThongTin();
			break;
		default: printf("Nhap lai\n"); break;
		}
	}
}

void nhapPhongBan() {
	int soPhongBan;

	string ten;
	string diaChi;
	int luongToiThieu;
	int soNhanVien;

	string maNhanVien;
	string hoVaTen;
	int namSinh;
	float heSoLuong;
	string chucDanh;

	cout << "Ban da chon nhap ds phong ban nhan vien!\n";
	cout << "Nhap so phong ban: ";

	cin >> soPhongBan;
	cin.ignore(256, '\n');

	for (int i = 1; i <= soPhongBan; ++i) {
		cout << "\nNhap thong tin cua phong ban thu " << i << ":\n";
		cout << "Nhap ten phong ban: ";
		getline(cin, ten);

		cout << "Nhap dia chi: ";
		getline(cin, diaChi);

		cout << "Nhap luong toi thieu: ";
		cin >> luongToiThieu;

		cout << "Nhap so nhan vien: ";
		cin >> soNhanVien;

		cin.ignore(256, '\n');

		PhongBan pb(ten, diaChi, luongToiThieu, soNhanVien);


		for (int j = 1; j <= soNhanVien; ++j) {
			cout << "Nhap nhan vien thu " << j << ":\n";

			cout << "\tNhap Ma nhan vien: ";
			getline(cin, maNhanVien);

			cout << "\tNhap Ten nhan vien: ";
			getline(cin, hoVaTen);

			cout << "\tNhap nam sinh: ";
			cin >> namSinh;

			cout << "\tNhap hs luong: ";
			cin >> heSoLuong;

			cin.ignore(256, '\n');

			cout << "\tNhap chuc danh: ";
			getline(cin, chucDanh);

			cout << '\n';

			pb.nhanVienList.emplace_back(maNhanVien, hoVaTen, namSinh, heSoLuong, chucDanh, luongToiThieu);
		}

		gPhongBanList.push_back(pb);
		cout << "\n";
	}

	cout << "Ban da nhap thanh cong!\n\n";
}

void inThongTin() {
	cout << "Ban da chon xuat ds Phong ban\n";
	for (PhongBan pb : gPhongBanList) {
		pb.getTongLuong();
		pb.getCd();
		cout << pb << "\n\n";
	}
}