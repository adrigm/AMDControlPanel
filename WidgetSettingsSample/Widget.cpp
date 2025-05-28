#include "pch.h"
#include "Widget.h"
#include "Widget.g.cpp"

#include <sstream>
using namespace winrt;
using namespace winrt::Windows::Foundation;
using namespace winrt::Windows::UI::Xaml;
using namespace winrt::Windows::UI::Xaml::Navigation;
using namespace winrt::Windows::UI::Xaml::Controls::Primitives;
using namespace Microsoft::Gaming::XboxGameBar;
using namespace adlx; // ADLX namespace

namespace winrt::WidgetSettingsSample::implementation
{
    // ────────────────────────────────────────────────────────────────────────────────
    // Construction / Destruction
    // ────────────────────────────────────────────────────────────────────────────────
    Widget::Widget()
    {
        InitializeComponent();
    }

    Widget::~Widget()
    {
        ReleaseADLX();
    }

    // ────────────────────────────────────────────────────────────────────────────────
    // Page Life‑cycle
    // ────────────────────────────────────────────────────────────────────────────────
    void Widget::OnNavigatedTo(NavigationEventArgs e)
    {
        m_widget = e.Parameter().as<XboxGameBarWidget>();
        m_widgetControl = XboxGameBarWidgetControl(m_widget);
        m_settingsToken = m_widget.SettingsClicked({ this, &Widget::SettingsButton_Click });

        InitializeADLX();
        RefreshUIStates();
    }

    void Widget::OnNavigatedFrom(NavigationEventArgs)
    {
        m_widget.SettingsClicked(m_settingsToken); // unhook
        ReleaseADLX();
    }

    // ────────────────────────────────────────────────────────────────────────────────
    // Status helper
    // ────────────────────────────────────────────────────────────────────────────────
    void Widget::SetStatus(const winrt::hstring& msg)
    {
        // UI thread already
        txtStatus().Text(msg);
        // Also emit to Output window for debugging
        ::OutputDebugStringW(msg.c_str());
        ::OutputDebugStringW(L"\n");
    }

    // ────────────────────────────────────────────────────────────────────────────────
    // ADLX Initialization / Tear‑down
    // ────────────────────────────────────────────────────────────────────────────────
    void Widget::InitializeADLX()
    {
        if (m_adlxReady)
            return;

        ADLX_RESULT res = m_adlxHelper.Initialize();
        if (ADLX_FAILED(res))
        {
            std::wstringstream ss; ss << L"ADLX init failed (" << res << L")";
            SetStatus(ss.str().c_str());
            return;
        }

        auto sysServices = m_adlxHelper.GetSystemServices();
        if (!sysServices)
        {
            SetStatus(L"GetSystemServices returned nullptr");
            return;
        }

        // GPU list
        if (ADLX_FAILED(sysServices->GetGPUs(&m_gpuList)) || !m_gpuList || m_gpuList->Empty())
        {
            SetStatus(L"No GPUs detected via ADLX");
            return;
        }
        m_gpuList->At(0, &m_gpu);
        if (!m_gpu)
        {
            SetStatus(L"Could not acquire first GPU");
            return;
        }

        // 3D service layers
        if (ADLX_FAILED(sysServices->Get3DSettingsServices(&m_3DServices)))
        {
            SetStatus(L"Failed to get 3DSettingsServices");
            return;
        }
        m_3DServices1 = IADLX3DSettingsServices1Ptr(m_3DServices);
        m_3DServices2 = IADLX3DSettingsServices2Ptr(m_3DServices);

        // AFMF
        if (m_3DServices1)
        {
            m_3DServices1->GetAMDFluidMotionFrames(&m_afmf);
            if (m_afmf)
            {
                adlx_bool supported = false;
                if (ADLX_SUCCEEDED(m_afmf->IsSupported(&supported)) && !supported)
                    m_afmf.Release();
            }
        }

        // RIS
        if (m_3DServices)
            m_3DServices->GetImageSharpening(m_gpu, &m_imageSharpen);
        if (m_3DServices2)
            m_3DServices2->GetImageSharpenDesktop(m_gpu, &m_imageSharpenDesktop);

        if (m_imageSharpen)
            m_imageSharpen->GetSharpnessRange(&m_sharpRange);

        m_adlxReady = true;
        SetStatus(L"ADLX initialized OK");
    }

    void Widget::ReleaseADLX()
    {
        if (!m_adlxReady)
            return;

        m_afmf.Release();
        m_imageSharpen.Release();
        m_imageSharpenDesktop.Release();
        m_3DServices.Release();
        m_3DServices1.Release();
        m_3DServices2.Release();
        m_gpu.Release();
        m_gpuList.Release();

        m_adlxHelper.Terminate();
        m_adlxReady = false;
    }

    // ────────────────────────────────────────────────────────────────────────────────
    // UI Helpers
    // ────────────────────────────────────────────────────────────────────────────────
    void Widget::RefreshUIStates()
    {
        if (!m_adlxReady)
            return;

        // AFMF
        if (m_afmf)
        {
            adlx_bool enabled = false;
            if (ADLX_SUCCEEDED(m_afmf->IsEnabled(&enabled)))
                swAFMF().IsOn(enabled);
            else
                SetStatus(L"AFMF: IsEnabled failed");
        }
        else
        {
            swAFMF().IsOn(false);
            SetStatus(L"AFMF not supported on this GPU/driver");
        }

        // RIS
        if (m_imageSharpen)
        {
            adlx_bool enabled = false;
            if (ADLX_SUCCEEDED(m_imageSharpen->IsEnabled(&enabled)))
            {
                swRIS().IsOn(enabled);
                slRISSharpening().Visibility(enabled ? Visibility::Visible : Visibility::Collapsed);

                adlx_int sharp = 0;
                if (ADLX_SUCCEEDED(m_imageSharpen->GetSharpness(&sharp)))
                    slRISSharpening().Value(static_cast<double>(sharp));

                slRISSharpening().Minimum(static_cast<double>(m_sharpRange.minValue));
                slRISSharpening().Maximum(static_cast<double>(m_sharpRange.maxValue));
                slRISSharpening().TickFrequency(static_cast<double>(m_sharpRange.step));
            }
            else
                SetStatus(L"RIS: IsEnabled failed");
        }
        else
        {
            swRIS().IsOn(false);
            slRISSharpening().Visibility(Visibility::Collapsed);
            SetStatus(L"Image Sharpening not supported on this GPU/driver");
        }
    }

    // ────────────────────────────────────────────────────────────────────────────────
    // Event Handlers
    // ────────────────────────────────────────────────────────────────────────────────
    void Widget::btnAfmfClick(IInspectable const&, RoutedEventArgs const&)
    {
        if (!m_afmf)
            return;

        bool target = !swAFMF().IsOn();
        ADLX_RESULT res = m_afmf->SetEnabled(target);
        if (ADLX_SUCCEEDED(res))
        {
            swAFMF().IsOn(target);
            SetStatus(target ? L"AFMF enabled" : L"AFMF disabled");
        }
        else
        {
            std::wstringstream ss; ss << L"AFMF SetEnabled failed (" << res << L")";
            SetStatus(ss.str().c_str());
        }
    }

    void Widget::btnRisClick(IInspectable const&, RoutedEventArgs const&)
    {
        if (!m_imageSharpen)
            return;

        bool target = !swRIS().IsOn();
        ADLX_RESULT res1 = m_imageSharpen->SetEnabled(target);
        ADLX_RESULT res2 = ADLX_RESULT::ADLX_OK;
        if (m_imageSharpenDesktop)
            res2 = m_imageSharpenDesktop->SetEnabled(target);

        if (ADLX_SUCCEEDED(res1) && ADLX_SUCCEEDED(res2))
        {
            swRIS().IsOn(target);
            slRISSharpening().Visibility(target ? Visibility::Visible : Visibility::Collapsed);
            SetStatus(target ? L"RIS enabled" : L"RIS disabled");
        }
        else
        {
            std::wstringstream ss; ss << L"RIS SetEnabled failed ("
                << res1 << L"," << res2 << L")";
            SetStatus(ss.str().c_str());
        }
    }

    void Widget::slRISSharpeningValueChanged(IInspectable const&, RangeBaseValueChangedEventArgs const& args)
    {
        if (!m_imageSharpen || !swRIS().IsOn())
            return;

        adlx_int val = static_cast<adlx_int>(args.NewValue());
        ADLX_RESULT res = m_imageSharpen->SetSharpness(val);
        if (ADLX_SUCCEEDED(res))
        {
            std::wstringstream ss; ss << L"Sharpening = " << val;
            SetStatus(ss.str().c_str());
        }
        else
        {
            std::wstringstream ss; ss << L"SetSharpness failed (" << res << L")";
            SetStatus(ss.str().c_str());
        }
    }

    // ────────────────────────────────────────────────────────────────────────────────
    // Settings button (unchanged)
    // ────────────────────────────────────────────────────────────────────────────────
    Windows::Foundation::IAsyncAction Widget::SettingsButton_Click(IInspectable const&, IInspectable const&)
    {
        auto strong_this{ get_strong() };
        co_await m_widget.ActivateSettingsAsync();
    }
}