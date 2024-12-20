# Email PC Control

Ứng dụng Desktop cho phép điều khiển máy tính từ xa thông qua email. Sử dụng Gmail API để xử lý email và Windows API để thực thi các lệnh điều khiển.

## Yêu Cầu Hệ Thống

- Windows 10/11
- Visual Studio 2019/2022 với C++ desktop development workload
- vcpkg package manager
- Gmail account

## Cài Đặt Dependencies

1. Cài đặt vcpkg nếu chưa có:
```bash
git clone https://github.com/Microsoft/vcpkg.git
cd vcpkg
./bootstrap-vcpkg.bat
```

2. Tích hợp vcpkg với Visual Studio:
```bash
vcpkg integrate install
```

3. Cài đặt các thư viện cần thiết:
```bash
vcpkg install wxwidgets:x64-windows
vcpkg install curl:x64-windows
vcpkg install jsoncpp:x64-windows
vcpkg install opencv:x64-windows
```

## Cấu Hình Project

1. Clone repository:
```bash
git clone https://github.com/chisngyen/RemotePC-via-Email.git
```

2. Tạo file `client_secret.json` trong thư mục của project với nội dung lấy từ Google Cloud Console:
```json
{
    "web": {
        "client_id": "your-client-id",
        "client_secret": "your-client-secret"
    }
}
```

3. Mở project trong Visual Studio:
- Mở file solution (.sln)
- Chọn platform là x64
- Chọn configuration là Release hoặc Debug

## Cách Sử Dụng

1. Chạy ứng dụng từ Visual Studio hoặc file .exe đã build

2. Đăng nhập:
   - Nhập Gmail của bạn
   - Xác thực qua Google OAuth

3. Kết nối với máy tính từ xa:
   - Nhập IP và port của máy cần điều khiển
   - Click "Connect"

4. Các lệnh có sẵn:
   - list::app - Liệt kê ứng dụng đang chạy
   - list::process - Liệt kê processes
   - list::service - Liệt kê services
   - screenshot::capture - Chụp màn hình
   - camera::open/close - Điều khiển webcam
   - system::shutdown/restart/lock - Điều khiển hệ thống
   - file::get/delete - Lấy và xóa file
   - app::start/stop - Khởi động/dừng ứng dụng
   - service::start/stop - Khởi động/dừng dịch vụ
   - help::cmd - Liệt kê các câu lệnh

## Gửi Email Điều Khiển

1. Format email:
   - To: [your-gmail]
   - Subject: Mail Control
   - Content: [commands] - [IP]
   
   Ví dụ: `list::app; screenshot::capture - 192.168.1.100`

2. Các lệnh phải được phân cách bằng dấu chấm phẩy (;)

## Xử Lý Sự Cố

1. Lỗi kết nối:
   - Kiểm tra IP và port
   - Đảm bảo firewall cho phép kết nối

2. Lỗi xác thực:
   - Kiểm tra client_secret.json
   - Thử đăng nhập lại

3. Lỗi thực thi lệnh:
   - Kiểm tra quyền admin
   - Xem log trong ứng dụng
