// eduroam-config.cpp : Define o ponto de entrada para a aplicação.
//

#include "stdafx.h"
#include "eduroam-config.h"

#define MY_ENCODING_TYPE  (PKCS_7_ASN_ENCODING | X509_ASN_ENCODING)

// Global function for free handles...
void FreeHandles(HANDLE hfile, HANDLE hsection, HCERTSTORE hFileStore, void* pfx, PCCERT_CONTEXT pctx, HCERTSTORE pfxStore, HCERTSTORE myStore)
{

	if (myStore)
		CertCloseStore(myStore, 0);

	if (pfxStore)
		CertCloseStore(pfxStore, CERT_CLOSE_STORE_FORCE_FLAG);

	if (pctx)
		CertFreeCertificateContext(pctx);

	if (pfx)
		UnmapViewOfFile(pfx);

	if (hFileStore)
		CertCloseStore(hFileStore, 0);

	if (hsection)
		CloseHandle(hsection);

	if (INVALID_HANDLE_VALUE != hfile)
		CloseHandle(hfile);

}

// This function imports a CA certificate...
int ImportCACert(LPCWSTR m_pathCA)
{
	HCERTSTORE pfxStore = 0;
	HCERTSTORE myStore = 0;
	HCERTSTORE hFileStore = 0;
	HANDLE hsection = 0;
	void* pfx = NULL;
	HANDLE hfile = INVALID_HANDLE_VALUE;
	PCCERT_CONTEXT pctx = NULL;

	// Open it...
	hfile = CreateFile(m_pathCA, FILE_READ_DATA, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);

	if (INVALID_HANDLE_VALUE == hfile)
	{
		MessageBox(NULL, L"Certificate not found. Check that the path indicated is correct.", L"Certificado CA", MB_ICONERROR);
		return 0;
	}

	hsection = CreateFileMapping(hfile, 0, PAGE_READONLY, 0, 0, 0);

	if (!hsection)
	{
		MessageBox(NULL, L"Error in 'CreateFileMapping'", L"Certificado CA", MB_ICONERROR);
		FreeHandles(hfile, hsection, hFileStore, pfx, pctx, pfxStore, myStore);
		return 0;
	}

	pfx = MapViewOfFile(hsection, FILE_MAP_READ, 0, 0, 0);

	if (!pfx)
	{
		MessageBox(NULL, L"Error in 'MapViewOfFile'", L"Certificado CA", MB_ICONERROR);
		FreeHandles(hfile, hsection, hFileStore, pfx, pctx, pfxStore, myStore);
		return 0;
	}

	pctx = CertCreateCertificateContext(MY_ENCODING_TYPE, (BYTE*)pfx, GetFileSize(hfile, 0));

	if (pctx == NULL)
	{
		MessageBox(NULL, L"Error in 'CertCreateCertificateContext'", L"Certificado CA", MB_ICONERROR);
		FreeHandles(hfile, hsection, hFileStore, pfx, pctx, pfxStore, myStore);
		return 0;
	}

	// we open the store for the CA
	hFileStore = CertOpenStore(CERT_STORE_PROV_SYSTEM, 0, 0, CERT_STORE_OPEN_EXISTING_FLAG | CERT_SYSTEM_STORE_CURRENT_USER, L"Root");

	if (!hFileStore)
	{
		MessageBox(NULL, L"Error in 'CertOpenStore'", L"Certificado CA", MB_ICONERROR);
		FreeHandles(hfile, hsection, hFileStore, pfx, pctx, pfxStore, myStore);
		return 0;
	}

	if (!CertAddCertificateContextToStore(hFileStore, pctx, CERT_STORE_ADD_NEW, 0))
	{

		DWORD err = GetLastError();

		if (CRYPT_E_EXISTS != err)
		{
			MessageBox(NULL, L"Error en 'CertAddCertificateContextToStore'", L"Certificado CA", MB_ICONERROR);
			FreeHandles(hfile, hsection, hFileStore, pfx, pctx, pfxStore, myStore);
			return 0;
		}
	}
	FreeHandles(hfile, hsection, hFileStore, pfx, pctx, pfxStore, myStore);
	return 1;
}


int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hInstance);
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);
	UNREFERENCED_PARAMETER(nCmdShow);

	TCHAR dir[_MAX_DIR];
	TCHAR drive[_MAX_DRIVE];
	TCHAR path[_MAX_PATH];

	int nArgs;
	LPWSTR *szArglist = CommandLineToArgvW(GetCommandLineW(), &nArgs);
	_wsplitpath_s(szArglist[0], drive, _MAX_DRIVE, dir, _MAX_DIR, NULL, 0, NULL, 0);
	_wmakepath_s(path, _MAX_PATH, drive, dir, NULL, NULL);
	SetCurrentDirectory(path);

	int ret;

	ret = ImportCACert(L"globalsign.der");
	if (!ret)
	{
		MessageBox(NULL, L"Erro na instalação do certificado.", L"eduroam", MB_ICONERROR);
		return EXIT_FAILURE;
	}

	ret = system("%WINDIR%\\System32\\netsh wlan add profile filename=\"eduroam.xml\"");
	if (ret != 0)
	{
		MessageBox(NULL, L"Erro na importação do perfil de rede wireless.", L"eduroam", MB_ICONERROR);
		return EXIT_FAILURE;
	}
	MessageBox(NULL, L"Rede eduroam configurada com sucesso.", L"eduroam", MB_ICONINFORMATION);
	return EXIT_SUCCESS;
}
