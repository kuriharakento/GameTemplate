/**
 * @file ShadowMap.PS.hlsl
 * @brief シャドウマップ描画用ピクセルシェーダー
 * @details 深度バッファへの書き込みのみを行うため、色出力は不要
 */

// 入力構造体（頂点シェーダーからの出力）
struct PixelShaderInput
{
    float4 position : SV_POSITION;
};

// 深度のみ出力（色出力なし）
void main(PixelShaderInput input)
{
    // シャドウマップは深度バッファへの書き込みのみを行うため、
    // ピクセルシェーダーでは特別な処理は不要
    // SV_POSITIONのz値が自動的に深度バッファに書き込まれる
}
