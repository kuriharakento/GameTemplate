#pragma once
#include <memory>
#include <vector>
#include <string>
#include "application/gameObject/component/action/IWeaponComponent.h"

using GameObjectComponent::IWeaponComponent;

class GameObject;
class CameraManager;

/**
 * @brief 武器切り替えを管理するクラス
 *
 * プレイヤーが持つ複数の武器を管理し、切り替え処理を行う。
 */
class WeaponManager
{
public:
    /**
     * @brief コンストラクタ
     * @param owner 武器を持つプレイヤー
     */
    explicit WeaponManager(GameObject* owner);

    /**
     * @brief 武器を追加
     * @param weapon 追加する武器コンポーネント
     */
    void AddWeapon(IWeaponComponent* weapon);

    /**
     * @brief 次の武器に切り替え
     */
    void SwitchToNext();

    /**
     * @brief 前の武器に切り替え
     */
    void SwitchToPrevious();

    /**
     * @brief 指定インデックスの武器に切り替え
     * @param index 武器のインデックス（0始まり）
     */
    void SwitchTo(int index);

    /**
     * @brief 毎フレームの更新処理（入力チェック）
     */
    void Update();

    /**
     * @brief 現在の武器を取得
     * @return 現在装備中の武器。なければnullptr
     */
    IWeaponComponent* GetCurrentWeapon() const;

    /**
     * @brief 現在の武器インデックスを取得
     * @return 武器インデックス（0始まり）
     */
    int GetCurrentIndex() const { return currentIndex_; }

    /**
     * @brief 登録されている武器数を取得
     * @return 武器の総数
     */
    size_t GetWeaponCount() const { return weapons_.size(); }

private:
    // 武器を持つオブジェクト（プレイヤー）
    GameObject* owner_ = nullptr;
    // 登録された武器リスト（非所有ポインタ）
    std::vector<IWeaponComponent*> weapons_;
    // 現在選択中の武器インデックス
    int currentIndex_ = 0;
};
