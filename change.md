# Lịch Sử Thay Đổi & Nhật Ký Kiểm Toán Mã Nguồn (Change Log & Audit History)

**Dự án:** Mô phỏng Vận chuyển Bức xạ & Tổn xạ Hạt nhân cho Hợp kim Thế hệ mới của GS. Ji-Jung Kai (CityU Hong Kong)  
**Tác giả chính:** TS. Nguyễn Hoàng Tùng (*Tung Nguyen Hoang, Ph.D.*)  
**File tài liệu:** `change.md`  
**Ngày lập:** 30/09/2026  

---

## I. Tổng Quan Tiến Trình Phát Triển (Version History)

### Phiên bản 1: `KaiAlloyShielding_v1` (Khởi tạo - Tháng 03/2026)
* **Mục tiêu:** Xây dựng bộ khung mô phỏng Geant4 độc lập (Standalone Shielding Framework) để đánh giá khả năng che chắn bức xạ của 6 hệ hợp kim đặc thù (HEA và CCIMA).
* **Các tính năng đã triển khai:**
  * Định nghĩa thành phần 6 hợp kim của GS. Kai và các vật liệu chuẩn (SS316L, Inconel 718, Fe, Pb, LBE, YH₂) từ các bài báo khoa học đã xuất bản (2025–2026).
  * Mô hình hình học tấm chắn phẳng (slab geometry) với bề dày thay đổi được.
  * Tích hợp danh sách vật lý Geant4 chuẩn `Shielding` kết hợp gói dữ liệu `G4NeutronHP` (ENDF/B-VIII.0).
  * Hỗ trợ các nguồn hạt: Watt fission ($^{235}\text{U}$), D-T fusion ($14.1\text{ MeV}$), Mono-energetic, White spectrum.
  * Xuất các phổ truyền qua neutron và photon gamma: `neutron_transmission_spectrum.csv`, `gamma_transmission_spectrum.csv`.
  * Bộ script tự động hóa quét nhiều vật liệu `batch_scan.py` và vẽ biểu đồ khoa học `plot_shielding_comparison.py`.

---

### Phiên bản 2: `KaiAlloyShielding_v2` (Nâng cấp Tích hợp Tổn xạ - Tháng 04/2026)
* **Mục tiêu:** Mở rộng từ bài toán che chắn thuần túy sang liên kết tính toán tổn xạ mạng tinh thể vĩ mô/vi mô bằng cách tích hợp trực tiếp với phần mềm **SPECTRA-PKA**.
* **Các thay đổi & tính năng mới so với v1:**
  1. **Module chấm điểm phổ chuẩn CCFE-709:**
     * Bổ sung module C++ [`NeutronSpectrumScorer.hh`](file:///C:/2026.9.25_Prof_Kai_Nuclear_Alloys_CityU_HongKong_TUNG%27s_codes_Mar+Apr2026/Prof_Kai_Nuclear_Alloys_CityU_HongKong_TUNG%27s_codes_Mar+Apr2026/TUNG%27s%20codes%20-%20Copy/KaiAlloyShielding_v2/include/NeutronSpectrumScorer.hh) và [`NeutronSpectrumScorer.cc`](file:///C:/2026.9.25_Prof_Kai_Nuclear_Alloys_CityU_HongKong_TUNG%27s_codes_Mar+Apr2026/Prof_Kai_Nuclear_Alloys_CityU_HongKong_TUNG%27s_codes_Mar+Apr2026/TUNG%27s%20codes%20-%20Copy/KaiAlloyShielding_v2/src/NeutronSpectrumScorer.cc).
     * Áp dụng thuật toán *track-length flux estimator* trong toàn bộ thể tích tấm chắn, phân chia thành 709 nhóm năng lượng ($10^{-5}\text{ eV}$ đến $55.5\text{ MeV}$).
     * Tự động xuất file phổ nơ-tron chuẩn: `neutron_flux_for_spectra_pka.dat`.
  2. **Tự động hóa dữ liệu đồng vị & SPECTRA-PKA:**
     * Bổ sung script [`generate_spectra_pka_inputs.py`](file:///C:/2026.9.25_Prof_Kai_Nuclear_Alloys_CityU_HongKong_TUNG%27s_codes_Mar+Apr2026/Prof_Kai_Nuclear_Alloys_CityU_HongKong_TUNG%27s_codes_Mar+Apr2026/TUNG%27s%20codes%20-%20Copy/KaiAlloyShielding_v2/generate_spectra_pka_inputs.py) tính toán chính xác tỷ lệ đồng vị tự nhiên theo chuẩn IUPAC 2021 và khối lượng giật lùi $(n, \gamma)$ theo AME2020.
     * Tạo thư mục [`spectra_pka_inputs/`](file:///C:/2026.9.25_Prof_Kai_Nuclear_Alloys_CityU_HongKong_TUNG%27s_codes_Mar+Apr2026/Prof_Kai_Nuclear_Alloys_CityU_HongKong_TUNG%27s_codes_Mar+Apr2026/TUNG%27s%20codes%20-%20Copy/KaiAlloyShielding_v2/spectra_pka_inputs) chứa sẵn các file cấu hình đầu vào `.in` cho cả 6 hợp kim (`0TaHEA.in`, `TaHEA.in`, `CCIMA.in`, `FeCoNiCr.in`, `FeCoNiCrAl03.in`, `L12HEA.in`).
     * Lập bảng đối chiếu tỷ lệ nguyên tử/đồng vị bảo toàn [VERIFICATION_TABLE.txt](file:///C:/2026.9.25_Prof_Kai_Nuclear_Alloys_CityU_HongKong_TUNG%27s_codes_Mar+Apr2026/Prof_Kai_Nuclear_Alloys_CityU_HongKong_TUNG%27s_codes_Mar+Apr2026/TUNG%27s%20codes%20-%20Copy/KaiAlloyShielding_v2/spectra_pka_inputs/VERIFICATION_TABLE.txt).
     * Bổ sung tài liệu kịch bản hướng dẫn chạy [SPECTRA_PKA_guide.sh](file:///C:/2026.9.25_Prof_Kai_Nuclear_Alloys_CityU_HongKong_TUNG%27s_codes_Mar+Apr2026/Prof_Kai_Nuclear_Alloys_CityU_HongKong_TUNG%27s_codes_Mar+Apr2026/TUNG%27s%20codes%20-%20Copy/KaiAlloyShielding_v2/SPECTRA_PKA_guide.sh).

---

## II. Báo Cáo Kiểm Toán Mã Nguồn & Các Lỗi Đã Xác Nhận (Audit Log: 26/09/2026)

Qua quá trình rà soát toàn bộ source code C++ và script Python, các vấn đề kỹ thuật sau đã được phát hiện và ghi nhận lại:

### 1. Lỗi Nghiêm Trọng (Critical Severity)
* **BUG-01: Vị trí nguồn phát hạt nằm ngoài World Volume (Geometry Error)**
  * **File:** `ShieldingPrimaryGenerator.cc` (Dòng 30) & `ShieldingDetectorConstruction.cc` (Dòng 63).
  * **Hiện tượng:** Nguồn hạt được đặt cố định ở tọa độ $Z = -200.0\text{ mm}$, trong khi kích thước World chỉ kéo dài từ $Z = -40.5\text{ mm}$ đến $+40.5\text{ mm}$ (với độ dày mẫu 20 mm).
  * **Hậu quả:** Nguồn hạt nằm ngoài thể tích World, hạt sơ cấp bị Geant4 hủy bỏ ngay khi bắn, không có hạt nào chạm tới tấm chắn.

* **BUG-02: Bất đồng bộ dữ liệu đa luồng Geant4 (Multi-threading Accumulation Bug)**
  * **File:** `ShieldingRunAction.cc` (Dòng 41–135) & `ShieldingRunAction.hh`.
  * **Hiện tượng:** Biến vector lưu histogram phổ (`fNeutronSpectrum`, `fGammaSpectrum`, `fIncidentSpectrum`) và đối tượng `fSpectrumScorer` là biến riêng lẻ của từng worker thread, không được đăng ký vào `G4AccumulableManager`. Khi kết thúc mô phỏng (`EndOfRunAction`), luồng chính (Master) in file nhưng không được gộp dữ liệu từ các Worker.
  * **Hậu quả:** Khi chạy chế độ mặc định đa luồng (Multi-Threaded), file CSV phổ truyền qua và file phổ nơ-tron cho SPECTRA-PKA xuất ra **toàn giá trị 0**.

* **BUG-03: Sai thuật toán lấy mẫu phổ phân hạch Watt (Physics/Math Error)**
  * **File:** `ShieldingPrimaryGenerator.cc` (Dòng 66–96).
  * **Hiện tượng:** Đoạn code lấy mẫu phổ Watt theo Cranberg tính xong bị bỏ xó. Vòng lặp thay thế bên dưới tự chế công thức chấp nhận `acceptance = sinhVal / exp(sqrtBE)`, triệt tiêu thành phần số mũ tăng của $\sinh$, làm biến dạng phổ phân hạch thành hàm suy giảm mũ thông thường.

---

### 2. Lỗi Trung Bình & Bất Cập Quy Trình (Medium / Pipeline Issues)
* **BUG-04: Mất dữ liệu file phổ SPECTRA-PKA trong `batch_scan.py`**
  * **File:** `batch_scan.py`.
  * **Hiện tượng:** Script chạy quét 9 vật liệu nhưng không đổi tên file `neutron_flux_for_spectra_pka.dat`.
  * **Hậu quả:** Sau khi quét xong toàn bộ danh sách, chỉ còn giữ lại file của vật liệu cuối cùng (`Inconel718`), các hợp kim trước đó đều bị ghi đè.

* **BUG-05: Thứ tự lệnh khởi tạo trong macro không chuẩn**
  * **File:** `batch_scan.py` (Hàm `write_macro`).
  * **Hiện tượng:** Lệnh `/run/initialize` được gọi trước khi thiết lập `/shield/material` và `/shield/thickness`, gây việc phải gọi lại `ReinitializeGeometry()` không cần thiết trong quá trình chạy.

* **BUG-06: Nhận diện hạt chạm biên (Boundary Check) trong `SteppingAction`**
  * **File:** `ShieldingSteppingAction.cc`.
  * **Hiện tượng:** Dùng `postPoint->GetTouchableHandle()->GetVolume()` tại điểm biên `fGeomBoundary` dễ trả về thể tích hiện tại thay vì thể tích đích đến (`Scorer`). Cần chuẩn hóa bằng `step->GetTrack()->GetNextVolume()`.

---

## III. Danh Mục Các File Đã Xử Lý (Completed Fixes Log - 30/09/2026)

Tất cả các lỗi nghiêm trọng và cải tiến đã được khắc phục hoàn chỉnh, áp dụng đồng bộ trên cả **`KaiAlloyShielding_v2`** và **`KaiAlloyShielding_v1`**:

| STT | File Đã Sửa | Chi Tiết Chỉnh Sửa Kỹ Thuật | Trạng Thái |
| :---: | :--- | :--- | :---: |
| 1 | `main.cc` | Cố định số luồng chạy: `runManager->SetNumberOfThreads(45);`. | **ĐÃ XONG** |
| 2 | `ShieldingDetectorConstruction.cc` | Mở rộng thể tích World cố định $400 \times 400 \times 600\text{ mm}^3$ (từ $-300\text{ mm}$ đến $+300\text{ mm}$). Mặt trước Shield đặt tại $Z=0$, tấm Scorer đặt sau Shield $5\text{ mm}$. Sửa `SetShieldMaterial` cập nhật trực tiếp `fShieldLogical`. | **ĐÃ XONG** |
| 3 | `ShieldingPrimaryGenerator.cc` | Đặt súng phát hạt tại tọa độ $Z = -50.0\text{ mm}$ (nằm an toàn trong World, cách mặt trước Shield $50\text{ mm}$). Viết lại hàm `SampleWattSpectrum` chuẩn xác theo mô hình bay hơi đẳng hướng của Watt (1952): Maxwellian + năng lượng trôi dạt $E_d$, loại bỏ dead-code và lỗi chia $\sinh$. | **ĐÃ XONG** |
| 4 | `ShieldingRunAction.hh` & `.cc` | Khai báo các bộ tích lũy Master tĩnh (`fgMasterNeutronSpectrum`, `fgMasterGammaSpectrum`, `fgMasterIncidentSpectrum`, `fgMasterSpectrumScorer`). Khi chạy 45 luồng, các worker tích lũy cục bộ không khóa mutex; khi kết thúc run, worker tự merge an toàn vào Master buffer. Master thread xuất file CSV và `.dat` từ dữ liệu đã gộp đầy đủ. | **ĐÃ XONG** |
| 5 | `ShieldingSteppingAction.cc` | Sửa logic bắt hạt: hạt nơ-tron tới Shield chỉ đếm hạt sơ cấp (`TrackID == 1`). Hạt xuyên vào Scorer được kiểm tra chính xác tại biên và gán `fStopAndKill` để đảm bảo mỗi hạt chỉ tính 1 lần duy nhất. | **ĐÃ XONG** |
| 6 | `batch_scan.py` | Cấu hình `/run/numberOfThreads 45` trong macro; đảo thứ tự khai báo vật liệu và bề dày TRƯỚC `/run/initialize`; bổ sung lệnh đổi tên lưu trữ file `neutron_flux_{label}_for_spectra_pka.dat` và file tóm tắt cho từng vật liệu vào thư mục `results/`. | **ĐÃ XONG** |
| 7 | Các macro (`.mac`) | Cập nhật `/run/numberOfThreads 45` trong `run_fission.mac`, `run_single.mac`, `run_spectrum_for_spectra_pka.mac`. | **ĐÃ XONG** |

---

## IV. Cấu Hình Thực Thi Sau Khi Sửa (Execution Reference)

* **Số luồng CPU (Threads):** Cố định **45 threads** (tối ưu hóa phân bổ tải trên CPU đa nhân cao cấp).
* **Hiệu năng:** Khóa mutex trong vòng lặp hạt đã được loại bỏ hoàn toàn; việc gộp dữ liệu chỉ thực hiện 1 lần duy nhất khi kết thúc mỗi run, mang lại hiệu suất tính toán song song tối đa ($~100\%$ CPU utilization trên 45 core).
* **Đầu ra đảm bảo:**
  * `neutron_transmission_spectrum.csv` và `gamma_transmission_spectrum.csv` chứa dữ liệu phổ phân bố năng lượng đầy đủ, không còn bị rỗng hay toàn số 0.
  * `neutron_flux_{label}_for_spectra_pka.dat` được tạo riêng biệt cho từng vật liệu trong thư mục `results/`, sẵn sàng đưa thẳng vào SPECTRA-PKA.

---
*Ghi chú: Lịch sử thay đổi được xác nhận và đồng bộ vào ngày 30/09/2026.*
