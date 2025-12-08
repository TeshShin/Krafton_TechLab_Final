#include "pch.h"
#include "SProgressBar.h"
#include "FD2DRenderer.h"

// =====================================================
// 렌더링
// =====================================================

void SProgressBar::Paint(FD2DRenderer& Renderer, const FGeometry& Geometry)
{
    FVector2D Position = Geometry.AbsolutePosition;
    FVector2D Size = Geometry.LocalSize;

    // 현재 진행률 가져오기
    float CurrentPercent = GetPercent();
    CurrentPercent = FMath::Clamp(CurrentPercent, 0.0f, 1.0f);

    // 배경 그리기
    if (CornerRadius > 0.0f)
    {
        Renderer.DrawFilledRoundedRect(Position, Size, BackgroundColor, CornerRadius);
    }
    else
    {
        Renderer.DrawFilledRect(Position, Size, BackgroundColor);
    }

    // 채워진 부분 그리기
    if (CurrentPercent > 0.0f)
    {
        FSlateColor CurrentFillColor = GetCurrentFillColor();
        FVector2D FillSize = Size;
        FVector2D FillPosition = Position;

        if (bFillLeftToRight)
        {
            FillSize.X = Size.X * CurrentPercent;
        }
        else
        {
            FillSize.X = Size.X * CurrentPercent;
            FillPosition.X = Position.X + Size.X - FillSize.X;
        }

        if (CornerRadius > 0.0f)
        {
            // 둥근 모서리의 경우 클리핑 사용
            FSlateRect ClipRect = FSlateRect::FromPositionSize(FillPosition, FillSize);
            Renderer.PushClip(ClipRect);
            Renderer.DrawFilledRoundedRect(Position, Size, CurrentFillColor, CornerRadius);
            Renderer.PopClip();
        }
        else
        {
            Renderer.DrawFilledRect(FillPosition, FillSize, CurrentFillColor);
        }
    }

    // 테두리 그리기
    if (BorderThickness > 0.0f)
    {
        if (CornerRadius > 0.0f)
        {
            Renderer.DrawRoundedRect(Position, Size, BorderColor, CornerRadius, BorderThickness);
        }
        else
        {
            Renderer.DrawRect(Position, Size, BorderColor, BorderThickness);
        }
    }
}

// =====================================================
// 값 설정
// =====================================================

SProgressBar& SProgressBar::SetPercent(float InPercent)
{
    Percent = FMath::Clamp(InPercent, 0.0f, 1.0f);
    PercentGetter = nullptr;  // 직접 설정하면 바인딩 해제
    return *this;
}

SProgressBar& SProgressBar::SetValue(float Current, float Max)
{
    if (Max > 0.0f)
    {
        Percent = FMath::Clamp(Current / Max, 0.0f, 1.0f);
    }
    else
    {
        Percent = 0.0f;
    }
    PercentGetter = nullptr;
    return *this;
}

SProgressBar& SProgressBar::BindPercent(FFloatGetter InGetter)
{
    PercentGetter = InGetter;
    return *this;
}

float SProgressBar::GetPercent() const
{
    if (PercentGetter)
    {
        return PercentGetter();
    }
    return Percent;
}

// =====================================================
// 스타일 설정
// =====================================================

SProgressBar& SProgressBar::SetFillColor(const FSlateColor& InColor)
{
    FillColor = InColor;
    return *this;
}

SProgressBar& SProgressBar::SetBackgroundColor(const FSlateColor& InColor)
{
    BackgroundColor = InColor;
    return *this;
}

SProgressBar& SProgressBar::SetBorderColor(const FSlateColor& InColor)
{
    BorderColor = InColor;
    return *this;
}

SProgressBar& SProgressBar::SetBorderThickness(float InThickness)
{
    BorderThickness = InThickness;
    return *this;
}

SProgressBar& SProgressBar::SetCornerRadius(float InRadius)
{
    CornerRadius = InRadius;
    return *this;
}

SProgressBar& SProgressBar::SetLeftToRight(bool bLeftToRight)
{
    bFillLeftToRight = bLeftToRight;
    return *this;
}

SProgressBar& SProgressBar::SetWarningThreshold(float InThreshold)
{
    WarningThreshold = InThreshold;
    return *this;
}

SProgressBar& SProgressBar::SetWarningColor(const FSlateColor& InColor)
{
    WarningColor = InColor;
    return *this;
}

FSlateColor SProgressBar::GetCurrentFillColor() const
{
    if (WarningThreshold > 0.0f && GetPercent() < WarningThreshold)
    {
        return WarningColor;
    }
    return FillColor;
}
