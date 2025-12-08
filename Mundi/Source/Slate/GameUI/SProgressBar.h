#pragma once

/**
 * @file SProgressBar.h
 * @brief 진행 상황 바 위젯
 *
 * 산소 게이지, 물 게이지 등에 사용되는 진행 상황 바입니다.
 */

#include "SWidget.h"
#include <functional>

/**
 * @class SProgressBar
 * @brief 진행 상황 바 위젯
 */
class SProgressBar : public SWidget
{
public:
    using FFloatGetter = std::function<float()>;

    // =====================================================
    // 생성자
    // =====================================================

    SProgressBar() = default;
    virtual ~SProgressBar() override = default;

    // =====================================================
    // 렌더링
    // =====================================================

    void Paint(FD2DRenderer& Renderer, const FGeometry& Geometry) override;

    // =====================================================
    // 값 설정 (Fluent API)
    // =====================================================

    /** 진행률 직접 설정 (0.0 ~ 1.0) */
    SProgressBar& SetPercent(float InPercent);

    /** 현재 값과 최대 값으로 진행률 설정 */
    SProgressBar& SetValue(float Current, float Max);

    /** 동적 진행률 바인딩 (매 프레임 호출) */
    SProgressBar& BindPercent(FFloatGetter InGetter);

    /** 현재 진행률 가져오기 (0.0 ~ 1.0) */
    float GetPercent() const;

    // =====================================================
    // 스타일 설정 (Fluent API)
    // =====================================================

    /** 전경 색상 (채워진 부분) */
    SProgressBar& SetFillColor(const FSlateColor& InColor);
    const FSlateColor& GetFillColor() const { return FillColor; }

    /** 배경 색상 */
    SProgressBar& SetBackgroundColor(const FSlateColor& InColor);
    const FSlateColor& GetBackgroundColor() const { return BackgroundColor; }

    /** 테두리 색상 */
    SProgressBar& SetBorderColor(const FSlateColor& InColor);
    const FSlateColor& GetBorderColor() const { return BorderColor; }

    /** 테두리 두께 (0이면 테두리 없음) */
    SProgressBar& SetBorderThickness(float InThickness);
    float GetBorderThickness() const { return BorderThickness; }

    /** 둥근 모서리 반경 (0이면 각진 모서리) */
    SProgressBar& SetCornerRadius(float InRadius);
    float GetCornerRadius() const { return CornerRadius; }

    /** 진행 방향 (true: 왼쪽->오른쪽, false: 오른쪽->왼쪽) */
    SProgressBar& SetLeftToRight(bool bLeftToRight);

    /** 경고 임계값 설정 (이 값 미만이면 경고 색상 사용) */
    SProgressBar& SetWarningThreshold(float InThreshold);
    float GetWarningThreshold() const { return WarningThreshold; }

    /** 경고 색상 */
    SProgressBar& SetWarningColor(const FSlateColor& InColor);
    const FSlateColor& GetWarningColor() const { return WarningColor; }

    /** 현재 채우기 색상 가져오기 (경고 상태 고려) */
    FSlateColor GetCurrentFillColor() const;

private:
    // =====================================================
    // 값
    // =====================================================

    float Percent = 1.0f;
    FFloatGetter PercentGetter;

    // =====================================================
    // 스타일
    // =====================================================

    FSlateColor FillColor = FSlateColor(0.2f, 0.6f, 1.0f, 1.0f);  // 파란색
    FSlateColor BackgroundColor = FSlateColor(0.1f, 0.1f, 0.1f, 0.8f);  // 어두운 회색
    FSlateColor BorderColor = FSlateColor::White();
    float BorderThickness = 0.0f;
    float CornerRadius = 0.0f;
    bool bFillLeftToRight = true;

    // 경고 시스템
    float WarningThreshold = 0.0f;  // 0이면 경고 비활성화
    FSlateColor WarningColor = FSlateColor::Red();
};
